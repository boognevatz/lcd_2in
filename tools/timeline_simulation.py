#!/usr/bin/env python3

from __future__ import annotations

import argparse
import re
import sys
from dataclasses import dataclass, replace


BUCKET_NAMES = ("A", "B", "C")
TOKEN_RE = re.compile(r"^(?P<prefix>[~^+]?)(?:(?P<frame>\d+)(?P<half>[UL])|-)$")
CASE_RE = re.compile(
    r"^Buckets:\s*\((?P<buckets>[^)]*)\)\s*,\s*Hints:\s*\((?P<hints>[^)]*)\)"
    r"(?:\s*,\s*Steps:\s*(?P<steps>\d+))?\s*$"
)
EXPECTED_RE = re.compile(
    r"^(?P<step>\d+)\.\s*Buckets:\s*\((?P<buckets>[^)]*)\)\s*\|\s*Hints:\s*\((?P<hints>[^)]*)\)\s*$"
)


@dataclass(frozen=True)
class BucketState:
    name: str
    frame: int | None
    half: str | None
    dirty: bool
    cemented: bool
    empty: bool

    def render(self) -> str:
        if self.empty:
            base = "-"
        else:
            base = f"{self.frame}{self.half}"

        if self.dirty and self.cemented:
            return f"+{base}"
        if self.dirty:
            return f"~{base}"
        if self.cemented:
            return f"^{base}"
        return base


@dataclass(frozen=True)
class TimelineState:
    buckets: tuple[BucketState, BucketState, BucketState]
    hints: tuple[str, ...]

    def render_buckets(self) -> str:
        return ",".join(bucket.render() for bucket in self.buckets)

    def render_hints(self) -> str:
        return ",".join(self.hints) if self.hints else "-"


@dataclass(frozen=True)
class Transition:
    step: int
    completed_bucket: str
    completed_half: str
    dirty_target: str
    dirty_half: str
    consumed_hint: str | None
    next_cemented: str | None
    state: TimelineState


@dataclass(frozen=True)
class ExpectedStep:
    step: int
    buckets: str
    hints: str


@dataclass(frozen=True)
class Case:
    buckets_text: str
    hints_text: str
    steps: int | None
    expected: tuple[ExpectedStep, ...]


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
        frame=None if empty else int(frame),
        half=half,
        dirty=prefix in ("~", "+"),
        cemented=prefix in ("^", "+"),
        empty=empty,
    )


def parse_bucket_list(text: str) -> tuple[BucketState, BucketState, BucketState]:
    parts = [part.strip() for part in text.split(",") if part.strip()]
    if len(parts) != 3:
        raise ValueError("Buckets input must contain exactly 3 entries")
    parsed = [parse_bucket_token(token, name) for name, token in zip(BUCKET_NAMES, parts)]
    return parsed[0], parsed[1], parsed[2]


def parse_hints(text: str | None) -> tuple[str, ...]:
    if text is None:
        return ()

    parts = [part.strip() for part in text.split(",") if part.strip()]
    if not parts:
        return ()

    if all(part == "-" for part in parts):
        return ()

    hints: list[str] = []
    for part in parts:
        if part not in BUCKET_NAMES:
            raise ValueError(f"Invalid hint bucket: {part!r}")
        hints.append(part)
    return tuple(hints)


def normalize_argv(argv: list[str]) -> list[str]:
    normalized: list[str] = []
    i = 0
    while i < len(argv):
        arg = argv[i]
        if arg == "--hints" and i + 1 < len(argv) and argv[i + 1] == "-,-,-":
            normalized.append("--hints=-,-,-")
            i += 2
            continue
        normalized.append(arg)
        i += 1
    return normalized


def find_dirty_bucket(buckets: tuple[BucketState, BucketState, BucketState]) -> int:
    dirty = [idx for idx, bucket in enumerate(buckets) if bucket.dirty]
    if len(dirty) != 1:
        raise ValueError(f"Expected exactly one dirty bucket, found {len(dirty)}")
    return dirty[0]


def find_cemented_bucket(buckets: tuple[BucketState, BucketState, BucketState]) -> int:
    cemented = [idx for idx, bucket in enumerate(buckets) if bucket.cemented]
    if len(cemented) != 1:
        raise ValueError(f"Expected exactly one cemented bucket, found {len(cemented)}")
    return cemented[0]


def next_half(bucket: BucketState) -> tuple[int, str]:
    if bucket.frame is None or bucket.half is None:
        raise ValueError("Bucket must hold a half-frame")

    if bucket.dirty and bucket.cemented and bucket.half == "U":
        return bucket.frame + 1, "L"
    if bucket.half == "U":
        return bucket.frame, "L"
    return bucket.frame + 1, "U"


def simulate_one_step(state: TimelineState, step: int) -> Transition:
    dirty_idx = find_dirty_bucket(state.buckets)
    cemented_idx = find_cemented_bucket(state.buckets)
    dirty_bucket = state.buckets[dirty_idx]
    cemented_bucket = state.buckets[cemented_idx]

    if dirty_bucket.empty or dirty_bucket.frame is None or dirty_bucket.half is None:
        raise ValueError("Dirty bucket must hold a half-frame")
    if cemented_bucket.name not in BUCKET_NAMES:
        raise ValueError("Invalid cemented bucket")

    new_frame, new_half = next_half(dirty_bucket)
    consumed_hint = state.hints[0] if state.hints else None
    next_cemented_name = consumed_hint

    updated = [replace(bucket, dirty=False, cemented=False) for bucket in state.buckets]

    updated[dirty_idx] = replace(dirty_bucket, dirty=False, cemented=False)
    updated[cemented_idx] = BucketState(
        name=cemented_bucket.name,
        frame=new_frame,
        half=new_half,
        dirty=True,
        cemented=False,
        empty=False,
    )

    if next_cemented_name is not None:
        next_idx = BUCKET_NAMES.index(next_cemented_name)
        next_bucket = updated[next_idx]
        updated[next_idx] = replace(next_bucket, cemented=True)

    next_state = TimelineState(buckets=(updated[0], updated[1], updated[2]), hints=state.hints[1:])
    return Transition(
        step=step,
        completed_bucket=dirty_bucket.name,
        completed_half=f"{dirty_bucket.frame}{dirty_bucket.half}",
        dirty_target=cemented_bucket.name,
        dirty_half=f"{new_frame}{new_half}",
        consumed_hint=consumed_hint,
        next_cemented=next_cemented_name,
        state=next_state,
    )


def simulate(state: TimelineState, max_steps: int | None) -> list[Transition]:
    transitions: list[Transition] = []
    current = state
    step = 1
    while current.hints and (max_steps is None or step <= max_steps):
        transition = simulate_one_step(current, step)
        transitions.append(transition)
        current = transition.state
        step += 1
    return transitions


def print_timeline(initial: TimelineState, transitions: list[Transition]) -> None:
    print(f"Buckets: ({initial.render_buckets()})")
    print(f"Hints:   ({initial.render_hints()})")
    if not transitions:
        print("Timeline: no simulated transitions")
        print()
        return

    print("Timeline:")
    for transition in transitions:
        consumed = transition.consumed_hint if transition.consumed_hint is not None else "-"
        next_cemented = transition.next_cemented if transition.next_cemented is not None else "-"
        print(
            f"  {transition.step:02d}. complete {transition.completed_bucket}:{transition.completed_half}"
            f" -> dirty {transition.dirty_target}:{transition.dirty_half}"
            f" | use hint {consumed}"
            f" | next cemented {next_cemented}"
        )
        print(
            f"      Buckets: ({transition.state.render_buckets()})"
            f" | Hints: ({transition.state.render_hints()})"
        )
    print()


def render_expected_step(transition: Transition) -> str:
    return (
        f"{transition.step:02d}. Buckets: ({transition.state.render_buckets()})"
        f" | Hints: ({transition.state.render_hints()})"
    )


def parse_case_blocks(path: str) -> list[Case]:
    cases: list[Case] = []

    with open(path, "r", encoding="ascii") as handle:
        lines = handle.readlines()

    i = 0
    while i < len(lines):
        raw_line = lines[i]
        line = raw_line.strip()
        line_no = i + 1
        i += 1

        if not line or line.startswith("#"):
            continue

        match = CASE_RE.fullmatch(line)
        if not match:
            raise ValueError(f"Invalid case format at {path}:{line_no}: {raw_line.rstrip()}")

        expected: list[ExpectedStep] = []
        while i < len(lines):
            peek_raw = lines[i]
            peek = peek_raw.strip()
            if not peek:
                i += 1
                break
            if peek.startswith("#"):
                i += 1
                continue
            if CASE_RE.fullmatch(peek):
                break

            expected_match = EXPECTED_RE.fullmatch(peek)
            if not expected_match:
                raise ValueError(
                    f"Invalid expected timeline format at {path}:{i + 1}: {peek_raw.rstrip()}"
                )

            expected.append(
                ExpectedStep(
                    step=int(expected_match.group("step")),
                    buckets=expected_match.group("buckets"),
                    hints=expected_match.group("hints"),
                )
            )
            i += 1

        cases.append(
            Case(
                buckets_text=match.group("buckets"),
                hints_text=match.group("hints"),
                steps=int(match.group("steps")) if match.group("steps") else None,
                expected=tuple(expected),
            )
        )

    return cases


def run_case(
    buckets_text: str,
    hints_text: str,
    steps: int | None,
    expected: tuple[ExpectedStep, ...] = (),
) -> bool:
    initial = TimelineState(
        buckets=parse_bucket_list(buckets_text),
        hints=parse_hints(hints_text),
    )
    transitions = simulate(initial, steps)
    print_timeline(initial, transitions)

    if not expected:
        return True

    actual = tuple(render_expected_step(transition) for transition in transitions)
    expected_lines = tuple(
        f"{step.step:02d}. Buckets: ({step.buckets}) | Hints: ({step.hints})" for step in expected
    )
    ok = actual == expected_lines

    print("Expected:")
    for line in expected_lines:
        print(f"  {line}")
    print(f"Result:  {'PASS' if ok else 'FAIL'}")
    print()
    return ok


def run_case_file(path: str) -> int:
    cases = parse_case_blocks(path)
    passed = 0
    total = 0
    for case in cases:
        total += 1
        ok = run_case(case.buckets_text, case.hints_text, case.steps, case.expected)
        if ok:
            passed += 1
    print(f"Summary: {passed}/{total} passed")
    return 0 if passed == total else 1


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Simulate deterministic bucket-state timeline transitions from camera hints."
    )
    parser.add_argument("--buckets", help='Comma-separated buckets, e.g. "5U,5L,+10U"')
    parser.add_argument("--hints", help='Comma-separated hints, e.g. "A,B,C" or "-,-,-"')
    parser.add_argument("--steps", type=int, help="Optional maximum number of simulated transitions")
    parser.add_argument("--cases", help="Read cases from a file")
    return parser


def main() -> int:
    args = build_parser().parse_args(normalize_argv(sys.argv[1:]))
    if args.cases:
        return run_case_file(args.cases)
    if not args.buckets or args.hints is None:
        raise SystemExit("Use --buckets and --hints, or provide --cases")
    ok = run_case(args.buckets, args.hints, args.steps)
    return 0 if ok else 1


if __name__ == "__main__":
    raise SystemExit(main())
