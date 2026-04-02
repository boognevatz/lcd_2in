#!/usr/bin/env python3

from __future__ import annotations

import argparse
import re
import sys
from dataclasses import dataclass


BUCKET_NAMES = ("A", "B", "C")
TOKEN_RE = re.compile(r"^(?P<prefix>[~^+]?)(?:(?P<frame>\d+)(?P<half>[UL])|-)$")
CASE_RE = re.compile(
    r"^Mode:\s*(?P<mode>[a-z\-]+)\s*,\s*Buckets:\s*(?P<buckets>[^,]+,[^,]+,[^,]+)"
    r"(?:\s*,\s*Sent:\s*(?P<tx>\S+))?"
    r"(?:\s*,\s*Prev:\s*(?P<prev>[ABC],[ABC]))?"
    r"\s*,\s*TX:\s*(?P<expected>[ABC]\s*-\s*\d+[UL]\s*,\s*[ABC]\s*-\s*\d+[UL])\s*$"
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
    def complete(self) -> bool:
        return not self.empty and not self.dirty


@dataclass(frozen=True)
class SentHalf:
    frame: int
    half: str


@dataclass(frozen=True)
class TxChoice:
    upper_bucket: str
    upper_frame: int
    upper_half: str
    lower_bucket: str
    lower_frame: int
    lower_half: str

    def render(self) -> str:
        return (
            f"{self.upper_bucket} - {self.upper_frame}{self.upper_half},"
            f" {self.lower_bucket} - {self.lower_frame}{self.lower_half}"
        )


def normalize_argv(argv: list[str]) -> list[str]:
    normalized: list[str] = []
    i = 0
    while i < len(argv):
        arg = argv[i]
        if arg == "--tx" and i + 1 < len(argv) and argv[i + 1] == "-,-":
            normalized.append("--tx=-,-")
            i += 2
            continue
        normalized.append(arg)
        i += 1
    return normalized


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


def parse_sent_pair(text: str | None) -> list[SentHalf]:
    if text is None:
        return []
    parts = [part.strip().replace(" ", "") for part in text.split(",") if part.strip()]
    if len(parts) != 2:
        raise ValueError("TX input must contain exactly 2 entries")
    if all(token == "-" for token in parts):
        return []

    sent: list[SentHalf] = []
    for token in parts:
        match = re.fullmatch(r"(?P<frame>\d+)(?P<half>[UL])", token)
        if not match:
            raise ValueError(f"Invalid TX token: {token!r}")
        sent.append(SentHalf(frame=int(match.group("frame")), half=match.group("half")))
    return sent


def get_last_sent_frame(sent_pair: list[SentHalf]) -> int:
    if not sent_pair:
        return -1
    return max(item.frame for item in sent_pair)


def find_bucket(buckets: list[BucketState], frame: int, half: str) -> BucketState | None:
    for bucket in buckets:
        if not bucket.empty and bucket.frame == frame and bucket.half == half:
            return bucket
    return None


def make_choice(
    upper_bucket: BucketState,
    lower_bucket: BucketState,
    *,
    lower_frame: int | None = None,
    lower_half: str = "L",
) -> TxChoice:
    if upper_bucket.frame is None:
        raise RuntimeError("Upper bucket must have a frame")
    chosen_lower_frame = lower_bucket.frame if lower_frame is None else lower_frame
    if chosen_lower_frame is None:
        raise RuntimeError("Lower bucket must have a frame")
    return TxChoice(
        upper_bucket=upper_bucket.name,
        upper_frame=upper_bucket.frame,
        upper_half="U",
        lower_bucket=lower_bucket.name,
        lower_frame=chosen_lower_frame,
        lower_half=lower_half,
    )


def normalize_camera_slower_bucket(
    bucket: BucketState,
    dirty_upper_frame: int | None,
) -> BucketState:
    if bucket.empty or bucket.frame is None:
        return bucket

    frame = bucket.frame
    if bucket.half == "U" and bucket.cemented:
        frame += 1
    if bucket.half == "L" and bucket.cemented and dirty_upper_frame is not None:
        frame = dirty_upper_frame

    return BucketState(
        name=bucket.name,
        raw=bucket.raw,
        frame=frame,
        half=bucket.half,
        dirty=False,
        cemented=bucket.cemented,
        empty=False,
    )


def normalize_camera_slower_buckets(buckets: list[BucketState]) -> list[BucketState]:
    dirty_upper_frames = [bucket.frame for bucket in buckets if bucket.dirty and bucket.half == "U"]
    dirty_upper_frame = max(frame for frame in dirty_upper_frames if frame is not None) if dirty_upper_frames else None
    return [normalize_camera_slower_bucket(bucket, dirty_upper_frame) for bucket in buckets]


def select_pair_camera_slower(buckets: list[BucketState], sent_pair: list[SentHalf]) -> TxChoice:
    del sent_pair

    normalized = normalize_camera_slower_buckets(buckets)
    uppers = [bucket for bucket in normalized if not bucket.empty and bucket.half == "U"]
    lowers = [bucket for bucket in normalized if not bucket.empty and bucket.half == "L"]

    for upper in sorted(uppers, key=lambda bucket: (bucket.frame or -1, bucket.name), reverse=True):
        for lower in sorted(lowers, key=lambda bucket: (bucket.frame or -1, bucket.name), reverse=True):
            if upper.frame == lower.frame:
                return make_choice(upper, lower)

    raise RuntimeError("No same-frame TX pair available for camera-slower mode")


def select_pair_camera_faster(
    buckets: list[BucketState],
    prev_pair: tuple[str, str] | None,
) -> TxChoice:
    if prev_pair is None:
        return select_pair_camera_slower(buckets, [])

    just_finished = BUCKET_NAMES.index(prev_pair[1])
    cand_a = buckets[(just_finished + 1) % 3]
    cand_b = buckets[(just_finished + 2) % 3]

    a_upper = not cand_a.empty and cand_a.half == "U"
    b_upper = not cand_b.empty and cand_b.half == "U"

    if a_upper and not b_upper:
        lower_frame = cand_a.frame if cand_a.frame is not None else cand_b.frame
        return make_choice(cand_a, cand_b, lower_frame=lower_frame)
    if b_upper and not a_upper:
        lower_frame = cand_b.frame if cand_b.frame is not None else cand_a.frame
        return make_choice(cand_b, cand_a, lower_frame=lower_frame)

    fa = cand_a.frame if cand_a.frame is not None else -1
    fb = cand_b.frame if cand_b.frame is not None else -1
    if fb > fa:
        lower_frame = cand_b.frame if cand_b.frame is not None else cand_a.frame
        return make_choice(cand_b, cand_a, lower_frame=lower_frame)
    lower_frame = cand_a.frame if cand_a.frame is not None else cand_b.frame
    return make_choice(cand_a, cand_b, lower_frame=lower_frame)


def compute_tx_pair(
    buckets: list[BucketState],
    mode: str,
    tx_text: str | None,
    prev_pair_text: str | None,
) -> TxChoice:
    sent_pair = parse_sent_pair(tx_text)
    if mode == "camera-slower":
        return select_pair_camera_slower(buckets, sent_pair)

    prev_pair = None
    if prev_pair_text:
        parts = [part.strip() for part in prev_pair_text.split(",")]
        if len(parts) != 2 or any(part not in BUCKET_NAMES for part in parts):
            raise ValueError("Prev pair must look like A,B")
        prev_pair = (parts[0], parts[1])
    return select_pair_camera_faster(buckets, prev_pair)


def run_case(
    buckets_text: str,
    mode: str,
    tx_text: str | None,
    prev_pair_text: str | None,
    expected: str | None,
) -> bool:
    buckets = parse_bucket_list(buckets_text)
    pair = compute_tx_pair(buckets, mode, tx_text, prev_pair_text)
    pair_text = pair.render()

    print(f"Buckets: ({buckets_text})")
    print(f"Mode:    {mode}")
    if tx_text is not None:
        print(f"TX:      ({tx_text})")
    if prev_pair_text is not None:
        print(f"Prev:    ({prev_pair_text})")
    print(f"TX:      {pair_text}")

    if expected is None:
        print()
        return True

    ok = pair_text == expected
    print(f"Expect:  {expected}")
    print(f"Result:  {'PASS' if ok else 'FAIL'}")
    print()
    return ok


def run_case_file(path: str) -> int:
    passed = 0
    total = 0
    with open(path, "r", encoding="ascii") as handle:
        for line_no, raw_line in enumerate(handle, start=1):
            line = raw_line.strip()
            if not line or line.startswith("#"):
                continue
            match = CASE_RE.fullmatch(line)
            if not match:
                raise ValueError(f"Invalid case format at {path}:{line_no}: {raw_line.rstrip()}")
            total += 1
            ok = run_case(
                match.group("buckets"),
                match.group("mode"),
                match.group("tx"),
                match.group("prev"),
                match.group("expected"),
            )
            if ok:
                passed += 1
    print(f"Summary: {passed}/{total} passed")
    return 0 if passed == total else 1


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description="Simulate TX pair selection from bucket state.")
    parser.add_argument("--buckets", help='Comma-separated buckets, e.g. "~7U,^5L,6L"')
    parser.add_argument(
        "--mode",
        default="camera-slower",
        choices=("camera-slower", "camera-faster"),
        help="Selection mode",
    )
    parser.add_argument("--tx", help='Optional TX halves, e.g. "5U,5L" or "-,-"')
    parser.add_argument("--prev", help='Optional previous pair, e.g. "A,B"')
    parser.add_argument("--expected", help='Expected pair, e.g. "A - 7U, B - 7L"')
    parser.add_argument("--cases", help="Read cases from a file")
    return parser


def main() -> int:
    args = build_parser().parse_args(normalize_argv(sys.argv[1:]))
    if args.cases:
        return run_case_file(args.cases)
    if not args.buckets:
        raise SystemExit("Use --buckets or provide --cases")
    ok = run_case(args.buckets, args.mode, args.tx, args.prev, args.expected)
    return 0 if ok else 1


if __name__ == "__main__":
    raise SystemExit(main())
