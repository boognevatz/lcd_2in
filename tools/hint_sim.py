#!/usr/bin/env python3

from __future__ import annotations

import argparse
import re
import sys
from dataclasses import dataclass


BUCKET_NAMES = ("A", "B", "C")
TOKEN_RE = re.compile(r"^(?P<prefix>[~^+]?)(?:(?P<frame>\d+)(?P<half>[UL])|-)$")
CASE_RE = re.compile(
    r"^Buckets:\s*\((?P<buckets>[^)]*)\)\s*,\s*TX:\s*\((?P<tx>[^)]*)\)"
    r"(?:\s*=>\s*(?P<expected>[ABC],[ABC],[ABC]))?\s*$"
)


@dataclass(frozen=True)
class BucketState:
    name: str
    raw: str
    frame: int | None
    half: str | None
    dirty: bool
    cemented: bool
    empty: bool

    @property
    def dirty_cemented_upper(self) -> bool:
        return self.dirty and self.cemented and self.half == "U"


@dataclass(frozen=True)
class SentHalf:
    frame: int
    half: str


def get_sent_keys(sent_pair: list[SentHalf]) -> set[tuple[int, str]]:
    return {(item.frame, item.half) for item in sent_pair}


def get_last_sent_frame(sent_pair: list[SentHalf]) -> int:
    if not sent_pair:
        return -1
    return max(item.frame for item in sent_pair)


@dataclass(frozen=True)
class BucketMeaning:
    bucket: BucketState
    already_sent: bool
    stable_next: bool
    invalid_future_lower: bool
    note: str


def parse_bucket_token(token: str, name: str) -> BucketState:
    cleaned = token.strip().replace(" ", "")
    match = TOKEN_RE.fullmatch(cleaned)
    if not match:
        raise ValueError(f"Invalid bucket token for {name}: {token!r}")

    prefix = match.group("prefix") or ""
    frame = match.group("frame")
    half = match.group("half")
    empty = frame is None

    return BucketState(
        name=name,
        raw=cleaned,
        frame=None if empty else int(frame),
        half=half,
        dirty=prefix in ("~", "+"),
        cemented=prefix in ("^", "+"),
        empty=empty,
    )


def parse_bucket_list(text: str) -> list[BucketState]:
    parts = [part.strip() for part in text.split(",") if part.strip()]
    if len(parts) != 3:
        raise ValueError("Buckets input must contain exactly 3 entries")
    return [parse_bucket_token(token, name) for name, token in zip(BUCKET_NAMES, parts)]


def parse_sent_pair(text: str) -> list[SentHalf]:
    parts = [part.strip().replace(" ", "") for part in text.split(",") if part.strip()]
    if len(parts) != 2:
        raise ValueError("TX input must contain exactly 2 entries")

    if all(token in ("-", "none", "NONE") for token in parts):
        return []

    parsed: list[SentHalf] = []
    for token in parts:
        match = re.fullmatch(r"(?P<frame>\d+)(?P<half>[UL])", token)
        if not match:
            raise ValueError(f"Invalid TX token: {token!r}")
        parsed.append(SentHalf(frame=int(match.group("frame")), half=match.group("half")))
    return parsed


def classify_bucket(bucket: BucketState, sent_pair: list[SentHalf]) -> BucketMeaning:
    sent_keys = get_sent_keys(sent_pair)
    last_sent_frame = get_last_sent_frame(sent_pair)

    if bucket.empty:
        return BucketMeaning(bucket, False, False, False, "empty")

    if bucket.frame is not None and (
        (bucket.frame, bucket.half) in sent_keys or bucket.frame <= last_sent_frame
    ):
        return BucketMeaning(bucket, True, False, False, "already sent/stale")

    if bucket.dirty_cemented_upper:
        return BucketMeaning(
            bucket,
            False,
            False,
            True,
            "dirty+c cemented upper; treat as future lower",
        )

    if bucket.frame is not None and bucket.frame == last_sent_frame + 1 and not bucket.dirty:
        return BucketMeaning(bucket, False, True, False, "stable next half")

    if bucket.dirty and bucket.half == "U":
        return BucketMeaning(bucket, False, False, True, "dirty upper; unstable")

    if bucket.dirty and bucket.half == "L":
        return BucketMeaning(bucket, False, False, True, "dirty lower; unstable")

    return BucketMeaning(bucket, False, False, False, "future complete but not next frame")


def detect_forming_pair_idle_bucket(buckets: list[BucketState]) -> str | None:
    dirty_uppers = [bucket for bucket in buckets if bucket.dirty and bucket.half == "U"]
    cemented = [bucket for bucket in buckets if bucket.cemented]

    if len(dirty_uppers) != 1:
        return None
    if len(cemented) != 1:
        return None

    upper_bucket = dirty_uppers[0]
    cemented_bucket = cemented[0]
    if upper_bucket.name == cemented_bucket.name:
        return None

    for bucket in buckets:
        if bucket.name not in (upper_bucket.name, cemented_bucket.name):
            return bucket.name
    return None


def detect_next_upper_next_lower_hints(
    buckets: list[BucketState], sent_pair: list[SentHalf]
) -> tuple[str, str, str] | None:
    dirty_cemented_lowers = [
        bucket for bucket in buckets if bucket.dirty and bucket.cemented and bucket.half == "L"
    ]

    if len(dirty_cemented_lowers) != 1:
        return None

    lower_bucket = dirty_cemented_lowers[0]
    sent_keys = get_sent_keys(sent_pair)

    same_frame_complete_upper = None
    for bucket in buckets:
        if bucket.name == lower_bucket.name or bucket.empty:
            continue
        if bucket.frame == lower_bucket.frame and bucket.half == "U" and not bucket.dirty:
            same_frame_complete_upper = bucket
            break

    if same_frame_complete_upper is None:
        return None
    if (same_frame_complete_upper.frame, same_frame_complete_upper.half) not in sent_keys:
        return None

    third_bucket = None
    for bucket in buckets:
        if bucket.name not in (lower_bucket.name, same_frame_complete_upper.name):
            third_bucket = bucket.name
            break

    if third_bucket is None:
        return None

    return third_bucket, same_frame_complete_upper.name, same_frame_complete_upper.name


def detect_unusable_dirty_cemented_upper_hints(
    buckets: list[BucketState], sent_pair: list[SentHalf]
) -> tuple[str, str, str] | None:
    sent_keys = get_sent_keys(sent_pair)
    dirty_cemented_uppers = [
        bucket for bucket in buckets if bucket.dirty and bucket.cemented and bucket.half == "U"
    ]

    if len(dirty_cemented_uppers) != 1:
        return None

    unusable = dirty_cemented_uppers[0]
    if (unusable.frame, unusable.half) not in sent_keys:
        return None

    others = [bucket for bucket in buckets if bucket.name != unusable.name]
    ordered = sorted(
        others,
        key=lambda bucket: (
            bucket.frame if bucket.frame is not None else 1_000_000,
            0 if bucket.half == "L" else 1,
            BUCKET_NAMES.index(bucket.name),
        ),
    )
    return ordered[0].name, ordered[1].name, unusable.name


def detect_dirty_lower_with_cemented_lower_hints(
    buckets: list[BucketState], sent_pair: list[SentHalf]
) -> tuple[str, str, str] | None:
    sent_keys = get_sent_keys(sent_pair)
    dirty_lowers = [bucket for bucket in buckets if bucket.dirty and bucket.half == "L"]
    cemented_lowers = [bucket for bucket in buckets if bucket.cemented and bucket.half == "L"]

    if len(dirty_lowers) != 1 or len(cemented_lowers) != 1:
        return None

    dirty_lower = dirty_lowers[0]
    cemented_lower = cemented_lowers[0]
    if dirty_lower.name == cemented_lower.name:
        return None

    matching_upper = None
    for bucket in buckets:
        if bucket.name in (dirty_lower.name, cemented_lower.name) or bucket.empty:
            continue
        if bucket.frame == dirty_lower.frame and bucket.half == "U" and not bucket.dirty:
            matching_upper = bucket
            break

    if matching_upper is None:
        return None
    if (matching_upper.frame, matching_upper.half) not in sent_keys:
        return None

    return matching_upper.name, dirty_lower.name, dirty_lower.name


def detect_startup_complete_pair_hints(
    buckets: list[BucketState], sent_pair: list[SentHalf]
) -> tuple[str, str, str] | None:
    if sent_pair:
        return None

    uppers = [bucket for bucket in buckets if not bucket.empty and bucket.half == "U" and not bucket.dirty]
    lowers = [bucket for bucket in buckets if not bucket.empty and bucket.half == "L" and not bucket.dirty]

    if len(uppers) != 1 or len(lowers) != 1:
        return None
    if uppers[0].frame != lowers[0].frame:
        return None

    for bucket in buckets:
        if bucket.name not in (uppers[0].name, lowers[0].name):
            return bucket.name, bucket.name, bucket.name
    return None


def compute_hints(buckets: list[BucketState], sent_pair: list[SentHalf]) -> tuple[str, str, str]:
    meanings = [classify_bucket(bucket, sent_pair) for bucket in buckets]

    startup_pair_hints = detect_startup_complete_pair_hints(buckets, sent_pair)
    if startup_pair_hints is not None:
        return startup_pair_hints

    idle_bucket = detect_forming_pair_idle_bucket(buckets)
    if idle_bucket is not None:
        return idle_bucket, idle_bucket, idle_bucket

    projected_hints = detect_next_upper_next_lower_hints(buckets, sent_pair)
    if projected_hints is not None:
        return projected_hints

    unusable_upper_hints = detect_unusable_dirty_cemented_upper_hints(buckets, sent_pair)
    if unusable_upper_hints is not None:
        return unusable_upper_hints

    dirty_lower_hints = detect_dirty_lower_with_cemented_lower_hints(buckets, sent_pair)
    if dirty_lower_hints is not None:
        return dirty_lower_hints

    sent_order = []
    for half in sent_pair:
        for bucket in buckets:
            if bucket.empty:
                continue
            if bucket.frame == half.frame and bucket.half == half.half:
                if bucket.name not in sent_order:
                    sent_order.append(bucket.name)

    remaining = [meaning for meaning in meanings if meaning.bucket.name not in sent_order]

    def rank(meaning: BucketMeaning) -> tuple[int, int]:
        bucket = meaning.bucket
        if meaning.already_sent:
            return (0, BUCKET_NAMES.index(bucket.name))
        if bucket.empty:
            return (1, BUCKET_NAMES.index(bucket.name))
        if meaning.invalid_future_lower:
            return (2, BUCKET_NAMES.index(bucket.name))
        if meaning.stable_next:
            return (3, BUCKET_NAMES.index(bucket.name))
        return (4, BUCKET_NAMES.index(bucket.name))

    remaining_names = [meaning.bucket.name for meaning in sorted(remaining, key=rank)]
    hints = sent_order + remaining_names

    if len(hints) != 3 or len(set(hints)) != 3:
        raise RuntimeError(f"Internal error building hints: {hints}")

    return hints[0], hints[1], hints[2]


def format_meaning(meaning: BucketMeaning) -> str:
    flags = []
    if meaning.bucket.dirty:
        flags.append("dirty")
    if meaning.bucket.cemented:
        flags.append("cemented")
    if meaning.already_sent:
        flags.append("already-sent")
    if meaning.stable_next:
        flags.append("stable-next")
    if meaning.invalid_future_lower:
        flags.append("future-lower")
    suffix = " | ".join(flags) if flags else "plain"
    return f"{meaning.bucket.name}: {meaning.bucket.raw:>4} -> {meaning.note} [{suffix}]"


def run_case(buckets_text: str, tx_text: str, expected: str | None, verbose: bool) -> bool:
    buckets = parse_bucket_list(buckets_text)
    sent_pair = parse_sent_pair(tx_text)
    hints = compute_hints(buckets, sent_pair)
    hint_text = ",".join(hints)

    print(f"Buckets: ({buckets_text})")
    print(f"TX:      ({tx_text})")
    if verbose:
        for meaning in [classify_bucket(bucket, sent_pair) for bucket in buckets]:
            print(f"  {format_meaning(meaning)}")
    print(f"Hints:   {hint_text}")

    if expected is None:
        print()
        return True

    ok = hint_text == expected
    print(f"Expect:  {expected}")
    print(f"Result:  {'PASS' if ok else 'FAIL'}")
    print()
    return ok


def run_case_file(path: str, verbose: bool) -> int:
    passed = 0
    total = 0

    with open(path, "r", encoding="ascii") as handle:
        for line_no, raw_line in enumerate(handle, start=1):
            line = raw_line.strip()
            if not line or line.startswith("#"):
                continue

            match = CASE_RE.fullmatch(line)
            if not match:
                raise ValueError(f"Invalid case format at {path}:{line_no}: {raw_line.rstrip()}" )

            total += 1
            ok = run_case(
                match.group("buckets"),
                match.group("tx"),
                match.group("expected"),
                verbose,
            )
            if ok:
                passed += 1

    print(f"Summary: {passed}/{total} passed")
    return 0 if passed == total else 1


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description="Simulate camera hints from bucket state.")
    parser.add_argument("--buckets", help='Comma-separated buckets, e.g. "5U,5L,+6U"')
    parser.add_argument(
        "--sent",
        help='Comma-separated TX halves, e.g. "5U,5L" or "-,-". Use quotes or `--sent=-,-`.',
    )
    parser.add_argument("--expected", help='Expected hints, e.g. "A,B,C"')
    parser.add_argument("--cases", help="Read cases from a file")
    parser.add_argument("--verbose", action="store_true", help="Show semantic reasoning")
    return parser


def normalize_argv(argv: list[str]) -> list[str]:
    normalized: list[str] = []
    i = 0
    while i < len(argv):
        arg = argv[i]
        if arg == "--sent" and i + 1 < len(argv) and argv[i + 1] == "-,-":
            normalized.append("--sent=-,-")
            i += 2
            continue
        normalized.append(arg)
        i += 1
    return normalized


def main() -> int:
    args = build_parser().parse_args(normalize_argv(sys.argv[1:]))

    if args.cases:
        return run_case_file(args.cases, args.verbose)

    if not args.buckets or not args.sent:
        raise SystemExit("Use --buckets and --sent, or provide --cases")

    ok = run_case(args.buckets, args.sent, args.expected, args.verbose)
    return 0 if ok else 1


if __name__ == "__main__":
    raise SystemExit(main())
