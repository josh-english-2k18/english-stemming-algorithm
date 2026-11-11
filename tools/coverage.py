#!/usr/bin/env python3
"""Run tests with Clang instrumentation and report library-only coverage."""
from pathlib import Path
import os
import shutil
import subprocess

ROOT = Path(__file__).resolve().parents[1]
BUILD = ROOT / "build/coverage"


def main() -> None:
    BUILD.mkdir(parents=True, exist_ok=True)
    for profile in BUILD.glob("*.profraw"):
        profile.unlink()
    clang = shutil.which("clang")
    cov = shutil.which("llvm-cov")
    profdata = shutil.which("llvm-profdata")
    if (not cov or not profdata) and shutil.which("xcrun"):
        cov = subprocess.check_output(["xcrun", "--find", "llvm-cov"], text=True).strip()
        profdata = subprocess.check_output(["xcrun", "--find", "llvm-profdata"], text=True).strip()
    if not clang or not cov or not profdata:
        raise SystemExit("coverage requires clang, llvm-cov, and llvm-profdata")
    subprocess.run(["make", "-B", f"BUILD_DIR={BUILD}", f"CC={clang}", "CXX=clang++",
                    "CFLAGS=-g -O0 -fprofile-instr-generate -fcoverage-mapping",
                    "CXXFLAGS=-g -O0 -fprofile-instr-generate -fcoverage-mapping",
                    "LDFLAGS=-fprofile-instr-generate", "test"],
                   cwd=ROOT, check=True,
                   env={**os.environ, "LLVM_PROFILE_FILE": str(BUILD / "%m-%p.profraw")})
    merged = BUILD / "coverage.profdata"
    profiles = sorted(BUILD.glob("*.profraw"))
    subprocess.run([profdata, "merge", "-sparse", *map(str, profiles), "-o", str(merged)], check=True)
    # Include the fault-injection binary, whose instrumented esa.c is a separate copy.
    common = [str(BUILD / "test_esa"), "-object", str(BUILD / "test_oom"),
              f"-instr-profile={merged}", str(ROOT / "esa.c")]
    subprocess.run([cov, "report", *common], cwd=ROOT, check=True)
    subprocess.run([cov, "show", *common, "-format=html",
                    f"-output-dir={BUILD / 'html'}", "-show-branches=count"], cwd=ROOT, check=True)
    print(f"Coverage detail: {BUILD / 'html/index.html'}")


if __name__ == "__main__":
    main()
