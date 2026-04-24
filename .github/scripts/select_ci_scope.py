#!/usr/bin/env python3
"""
Compute CI scope from changed files:
- whether build/test jobs should run
- whether to run full test suite or targeted test subset
- impacted test source files for clang-tidy
"""

from __future__ import annotations

import argparse
import pathlib
import re
import subprocess
import sys
from typing import Iterable


TEST_MACRO_PATTERN = re.compile(
    r"^\s*(?:TEST|TEST_F|TEST_P|TYPED_TEST|TYPED_TEST_P)\s*\(\s*([A-Za-z_][A-Za-z0-9_]*)\s*,"
)

ZERO_SHA = "0" * 40


def run_command(args: list[str], cwd: pathlib.Path) -> str:
    completed = subprocess.run(
        args,
        cwd=str(cwd),
        check=True,
        capture_output=True,
        text=True,
    )
    return completed.stdout


def changed_files(repo_root: pathlib.Path, base: str, head: str) -> list[str] | None:
    if not base or base == ZERO_SHA:
        return None
    try:
        output = run_command(["git", "diff", "--name-only", f"{base}..{head}"], repo_root)
    except subprocess.CalledProcessError:
        return None
    return [line.strip() for line in output.splitlines() if line.strip()]


def all_test_sources(repo_root: pathlib.Path) -> list[str]:
    tests_dir = repo_root / "tests"
    if not tests_dir.exists():
        return []
    return sorted(
        str(path.relative_to(repo_root)).replace("\\", "/")
        for path in tests_dir.glob("*_test.cpp")
    )


def suites_in_test_file(path: pathlib.Path) -> set[str]:
    suites: set[str] = set()
    try:
        content = path.read_text(encoding="utf-8")
    except OSError:
        return suites

    for line in content.splitlines():
        match = TEST_MACRO_PATTERN.match(line)
        if match:
            suites.add(match.group(1))
    return suites


def include_to_test_file(repo_root: pathlib.Path, include_path: str) -> str | None:
    stem = pathlib.Path(include_path).stem
    candidate = repo_root / "tests" / f"{stem}_test.cpp"
    if candidate.exists():
        return str(candidate.relative_to(repo_root)).replace("\\", "/")
    return None


def write_output(path: pathlib.Path, key: str, value: str) -> None:
    with path.open("a", encoding="utf-8") as handle:
        handle.write(f"{key}<<__EOF__\n{value}\n__EOF__\n")


def normalize_paths(paths: Iterable[str]) -> list[str]:
    return sorted({path.replace("\\", "/") for path in paths})


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--base", required=True)
    parser.add_argument("--head", required=True)
    parser.add_argument("--github-output", required=True)
    parser.add_argument("--repo-root", default=".")
    args = parser.parse_args()

    repo_root = pathlib.Path(args.repo_root).resolve()
    output_path = pathlib.Path(args.github_output)

    changed = changed_files(repo_root, args.base, args.head)

    if changed is None:
        test_files = all_test_sources(repo_root)
        suites: set[str] = set()
        for test_file in test_files:
            suites |= suites_in_test_file(repo_root / test_file)
        write_output(output_path, "build_required", "true")
        write_output(output_path, "run_all_tests", "true")
        write_output(output_path, "ctest_regex", "")
        write_output(output_path, "reason", "base commit unavailable; running full validation")
        write_output(output_path, "changed_files", "")
        write_output(output_path, "impacted_test_files", "\n".join(test_files))
        write_output(output_path, "impacted_suite_count", str(len(suites)))
        return 0

    relevant_prefixes = ("include/", "tests/", "thirdparty/", "cmake/")
    relevant_exact = {"CMakeLists.txt", ".clang-tidy"}
    relevant_files = [
        path
        for path in changed
        if path.startswith(relevant_prefixes) or path in relevant_exact
    ]

    if not relevant_files:
        write_output(output_path, "build_required", "false")
        write_output(output_path, "run_all_tests", "false")
        write_output(output_path, "ctest_regex", "")
        write_output(output_path, "reason", "no code/test/build files changed")
        write_output(output_path, "changed_files", "\n".join(normalize_paths(changed)))
        write_output(output_path, "impacted_test_files", "")
        write_output(output_path, "impacted_suite_count", "0")
        return 0

    run_all = False
    reasons: list[str] = []
    impacted_tests: set[str] = set()

    for path in relevant_files:
        normalized = path.replace("\\", "/")
        if normalized.startswith("tests/"):
            if normalized.endswith("_test.cpp"):
                impacted_tests.add(normalized)
            else:
                run_all = True
                reasons.append(f"non-test source under tests changed: {normalized}")
            continue

        if normalized.startswith("include/"):
            mapped = include_to_test_file(repo_root, normalized)
            if mapped is not None:
                impacted_tests.add(mapped)
            else:
                run_all = True
                reasons.append(f"include has no direct test mapping: {normalized}")
            continue

        run_all = True
        reasons.append(f"broad-impact file changed: {normalized}")

    if run_all:
        impacted_tests = set(all_test_sources(repo_root))

    suites: set[str] = set()
    for test_file in impacted_tests:
        suites |= suites_in_test_file(repo_root / test_file)

    if not run_all and not suites:
        run_all = True
        reasons.append("unable to infer impacted suites; running full validation")
        impacted_tests = set(all_test_sources(repo_root))
        suites.clear()
        for test_file in impacted_tests:
            suites |= suites_in_test_file(repo_root / test_file)

    ctest_regex = ""
    if not run_all:
        suite_regex = "|".join(sorted(re.escape(suite) for suite in suites))
        ctest_regex = f"(^|/)({suite_regex})\\."

    reason = "; ".join(reasons) if reasons else "targeted test scope from changed files"

    write_output(output_path, "build_required", "true")
    write_output(output_path, "run_all_tests", "true" if run_all else "false")
    write_output(output_path, "ctest_regex", ctest_regex)
    write_output(output_path, "reason", reason)
    write_output(output_path, "changed_files", "\n".join(normalize_paths(changed)))
    write_output(output_path, "impacted_test_files", "\n".join(sorted(impacted_tests)))
    write_output(output_path, "impacted_suite_count", str(len(suites)))
    return 0


if __name__ == "__main__":
    sys.exit(main())
