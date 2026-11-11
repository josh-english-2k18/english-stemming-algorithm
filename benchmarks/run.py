#!/usr/bin/env python3
"""Compile and measure ESA with reproducible corpora and raw timing samples."""
from __future__ import annotations

import argparse
import datetime as dt
import hashlib
import json
import math
import os
from pathlib import Path
import platform
import random
import shlex
import statistics
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
FLAGS = ["-O3", "-DNDEBUG"]


def run(command: list[str], **kwargs: object) -> subprocess.CompletedProcess[str]:
    return subprocess.run(command, cwd=ROOT, text=True, capture_output=True,
                          check=True, **kwargs)


def percentile(values: list[float], fraction: float) -> float:
    ordered = sorted(values)
    index = (len(ordered) - 1) * fraction
    low = math.floor(index)
    high = math.ceil(index)
    return ordered[low] + (ordered[high] - ordered[low]) * (index - low)


def summarize(raw: list[dict[str, object]], mode: str, workload: str,
              rounds: int) -> dict[str, object]:
    if len({(row["checksum"], row["fingerprint"]) for row in raw}) != 1:
        raise RuntimeError("nondeterministic benchmark output")
    timings = [row["elapsed_ns"] / row["operations"] for row in raw]
    median = statistics.median(timings)
    result: dict[str, object] = {
        "mode": mode, "workload": workload, "rounds": rounds,
        "samples": len(raw), "ns_per_operation": {
            "median": median, "mean": statistics.mean(timings),
            "min": min(timings), "max": max(timings),
            "p95_sample_mean": percentile(timings, 0.95),
            "stdev": statistics.stdev(timings) if len(raw) > 1 else 0,
        },
        "operations_per_second": 1e9 / median,
        "raw_samples": raw,
    }
    if mode != "init":
        result["input_mib_per_second"] = (1e9 / median * raw[0]["corpus_bytes"] /
                                          raw[0]["corpus_tokens"] / 1024**2)
    return result


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--samples", type=int, default=7)
    parser.add_argument("--rounds", type=int, default=5000,
                        help="passes over corpus per timed sample")
    parser.add_argument("--init-rounds", type=int, default=2000)
    parser.add_argument("--workloads", nargs="+", choices=["plain", "mixed", "long"],
                        default=["plain", "mixed", "long"])
    parser.add_argument("--baseline-ref", help="optional local Git commit of historical ESA")
    parser.add_argument("--output", type=Path, default=ROOT / "build/benchmark.json")
    args = parser.parse_args()
    if args.samples < 1 or not 1 <= args.rounds <= 10_000_000 or not 1 <= args.init_rounds <= 10_000_000:
        parser.error("samples must be positive; rounds must be in 1..10000000")
    compiler = shlex.split(os.environ.get("CC", "cc"))
    report: dict[str, object] = {
        "schema_version": 1,
        "recorded_at_utc": dt.datetime.now(dt.timezone.utc).isoformat(),
        "platform": platform.platform(), "architecture": platform.machine(),
        "compiler": run(compiler + ["--version"]).stdout.strip(),
        "flags": FLAGS, "clock": "CLOCK_MONOTONIC",
        "sampling": "round-robin with deterministically shuffled job order per round",
        "sampling_seed": 0xE5A,
        "corpus_sha256": hashlib.sha256((ROOT / "tests/corpus.tsv").read_bytes()).hexdigest(),
        "source_sha256": hashlib.sha256((ROOT / "esa.c").read_bytes()).hexdigest(),
        "source_files_sha256": {
            name: hashlib.sha256((ROOT / name).read_bytes()).hexdigest()
            for name in ("esa.c", "esa.h", "esa_rules.h", "benchmarks/bench_esa.c")
        },
        "notes": ["p95 is across whole-sample means, not per-token tail latency",
                  "timed stemming includes output checksum and, in alloc mode, allocation/free",
                  "historical outputs differ for corrected rules and normalization"],
        "results": [],
    }
    results: list[dict[str, object]] = []
    with tempfile.TemporaryDirectory(prefix="esa-benchmark-") as directory:
        temporary = Path(directory)
        current = temporary / "current"
        run(compiler + ["-std=c11", *FLAGS, "-I", str(ROOT),
                        str(ROOT / "esa.c"), str(ROOT / "benchmarks/bench_esa.c"),
                        "-o", str(current)])
        binaries = [("current", current, ["alloc", "buffer"])]
        if args.baseline_ref:
            commit = run(["git", "rev-parse", "--verify", args.baseline_ref + "^{commit}"]).stdout.strip()
            baseline_dir = temporary / "baseline"
            baseline_dir.mkdir()
            for name in ("esa.c", "esa.h", "esa_rules.h"):
                source = run(["git", "show", f"{commit}:{name}"]).stdout
                (baseline_dir / name).write_text(source)
            baseline = temporary / "historical"
            compilation = run(compiler + ["-std=gnu11", *FLAGS, "-DESA_BENCH_LEGACY",
                                           "-I", str(baseline_dir),
                                           str(baseline_dir / "esa.c"),
                                           str(ROOT / "benchmarks/bench_esa.c"),
                                           "-o", str(baseline)])
            report["baseline_commit"] = commit
            report["baseline_compiler_diagnostics"] = compilation.stderr.strip()
            binaries.append(("historical", baseline, ["alloc"]))
        jobs = []
        for label, binary, modes in binaries:
            for workload in args.workloads:
                for mode in modes:
                    jobs.append({"implementation": label, "binary": binary,
                                 "mode": mode, "workload": workload,
                                 "rounds": args.rounds, "raw": []})
            jobs.append({"implementation": label, "binary": binary,
                         "mode": "init", "workload": "plain",
                         "rounds": args.init_rounds, "raw": []})
        for job in jobs:
            job["command"] = [str(job["binary"]), job["mode"], str(job["rounds"]),
                              str(ROOT / "tests/corpus.tsv"), job["workload"]]
            run(job["command"])  # One discarded full-process warmup per job.
        generator = random.Random(report["sampling_seed"])
        for sample in range(args.samples):
            order = list(jobs)
            generator.shuffle(order)
            for job in order:
                job["raw"].append(json.loads(run(job["command"]).stdout))
            print(f"Completed sample round {sample + 1}/{args.samples}", flush=True)
        for job in jobs:
            result = summarize(job["raw"], job["mode"], job["workload"], job["rounds"])
            label = job["implementation"]
            result["implementation"] = label
            for previous in results:
                if (previous["implementation"] == label and
                    previous["workload"] == result["workload"] and
                    previous["mode"] != "init" and result["mode"] != "init" and
                    previous["raw_samples"][0]["fingerprint"] !=
                        result["raw_samples"][0]["fingerprint"]):
                    raise RuntimeError("alloc and buffer outputs differ")
            results.append(result)
            unit = "ns/init+free" if result["mode"] == "init" else "ns/token"
            print(f"{label:10} {result['workload']:5} {result['mode']:6} "
                  f"{result['ns_per_operation']['median']:9.2f} {unit}", flush=True)
    report["results"] = results
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2) + "\n")
    print(f"Raw samples and environment: {args.output}")


if __name__ == "__main__":
    try:
        main()
    except subprocess.CalledProcessError as error:
        print(error.stderr, file=sys.stderr)
        raise SystemExit(error.returncode) from error
