"""Regression checks for benchmark inputs, operation counts, and report integrity."""
from __future__ import annotations

import json
import os
from pathlib import Path
import shlex
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
CORPUS = ROOT / "tests/corpus.tsv"


class BenchmarkTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.temporary = tempfile.TemporaryDirectory(prefix="esa-tooling-tests-")
        cls.addClassCleanup(cls.temporary.cleanup)
        cls.directory = Path(cls.temporary.name)
        cls.binary = cls.directory / "benchmark"
        command = shlex.split(os.environ.get("CC", "cc"))
        subprocess.run(command + ["-std=c11", "-O1", "-I", str(ROOT),
                                  str(ROOT / "esa.c"),
                                  str(ROOT / "benchmarks/bench_esa.c"),
                                  "-o", str(cls.binary)], check=True,
                       capture_output=True, text=True)
        cls.words = [line.split("\t")[0] for line in CORPUS.read_text().splitlines()
                     if line and not line.startswith("#")]

    def invoke(self, mode: str, rounds: str, workload: str = "plain",
               corpus: Path = CORPUS) -> subprocess.CompletedProcess[str]:
        return subprocess.run([str(self.binary), mode, rounds, str(corpus), workload],
                              capture_output=True, text=True, check=False)

    def test_round_count_rejects_partial_signed_and_overflowing_values(self) -> None:
        for value in ("1oops", "+1", "-1", "-18446744073709551615", " 1", "",
                      "0", "10000001", "9999999999999999999999999999999999"):
            with self.subTest(value=value):
                result = self.invoke("alloc", value)
                self.assertNotEqual(result.returncode, 0)
                self.assertIn("invalid benchmark arguments", result.stderr)

    def test_corpus_rejects_invalid_tokens_and_overlong_rows(self) -> None:
        for row in ("Upper\tupper\n", "a-b\tab\n", "\tblank\n",
                    "abc\t" + "a" * 600 + "\n"):
            with self.subTest(row=row[:30]):
                corpus = self.directory / "invalid.tsv"
                corpus.write_text(row)
                result = self.invoke("alloc", "1", corpus=corpus)
                self.assertNotEqual(result.returncode, 0)
                self.assertTrue(result.stderr)

    def test_operation_counts_workload_sizes_and_output_fingerprints(self) -> None:
        for workload, extra_bytes in (("plain", 0), ("mixed", 5), ("long", 512)):
            fingerprints = []
            for mode in ("alloc", "buffer"):
                with self.subTest(workload=workload, mode=mode):
                    result = self.invoke(mode, "2", workload)
                    self.assertEqual(result.returncode, 0, result.stderr)
                    report = json.loads(result.stdout)
                    self.assertEqual(report["operations"], len(self.words) * 2)
                    self.assertEqual(report["corpus_tokens"], len(self.words))
                    self.assertEqual(report["corpus_bytes"],
                                     sum(len(word) + extra_bytes for word in self.words))
                    fingerprints.append(report["fingerprint"])
            self.assertEqual(fingerprints[0], fingerprints[1])
        result = self.invoke("init", "2")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(json.loads(result.stdout)["operations"], 2)

    def test_runner_rejects_invalid_counts_before_building(self) -> None:
        for arguments in (("--samples", "0"), ("--rounds", "0"),
                          ("--rounds", "10000001"), ("--init-rounds", "-1")):
            with self.subTest(arguments=arguments):
                result = subprocess.run([sys.executable, str(ROOT / "benchmarks/run.py"),
                                         *arguments], capture_output=True, text=True, check=False)
                self.assertEqual(result.returncode, 2)
                self.assertIn("rounds must be", result.stderr)

    def test_single_sample_runner_report(self) -> None:
        destination = self.directory / "report.json"
        result = subprocess.run([sys.executable, str(ROOT / "benchmarks/run.py"),
                                 "--samples", "1", "--rounds", "2", "--init-rounds", "2",
                                 "--workloads", "plain", "--output", str(destination)],
                                capture_output=True, text=True, check=False)
        self.assertEqual(result.returncode, 0, result.stderr)
        report = json.loads(destination.read_text())
        self.assertEqual(len(report["results"]), 3)
        for item in report["results"]:
            self.assertEqual(item["samples"], 1)
            self.assertEqual(item["ns_per_operation"]["stdev"], 0)
            sample = item["raw_samples"][0]
            mean = sample["elapsed_ns"] / sample["operations"]
            self.assertGreater(mean, 0)
            self.assertEqual(item["ns_per_operation"]["median"], mean)
            self.assertAlmostEqual(item["operations_per_second"], 1e9 / mean)
