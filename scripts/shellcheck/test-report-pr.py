#!/usr/bin/env python3
"""Integration tests for the non-blocking pull-request ShellCheck report."""

import os
import contextlib
import importlib.util
import io
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest


SOURCE = Path(__file__).resolve().parent
spec = importlib.util.spec_from_file_location("shellcheck_report", SOURCE / "report-pr.py")
shellcheck_report = importlib.util.module_from_spec(spec)
spec.loader.exec_module(shellcheck_report)


class ReportShellCheckTest(unittest.TestCase):
	def setUp(self):
		self.temp = tempfile.TemporaryDirectory(prefix="shellcheck-pr-")
		self.addCleanup(self.temp.cleanup)
		self.root = Path(self.temp.name)
		for directory in ("icons", "packaging", "scripts/shellcheck"):
			(self.root / directory).mkdir(parents=True)
		shutil.copyfile(SOURCE / "report-pr.py", self.root / "scripts/shellcheck/report-pr.py")
		shutil.copyfile(SOURCE / "run.sh", self.root / "scripts/shellcheck/run.sh")
		(self.root / "scripts/shellcheck/exempt.txt").write_text(
			"packaging/legacy.sh\nscripts/old.sh\nscripts/renamed.sh\nscripts/added.sh\nsubsurface.debug\n"
		)
		self.git("init", "-q", "-b", "master")
		self.git("config", "user.name", "Test")
		self.git("config", "user.email", "test@example.com")
		for name in ("icons/clean.sh", "packaging/legacy.sh", "scripts/old.sh", "scripts/delete.sh"):
			(self.root / name).write_text("#!/bin/sh\necho hello\n")
		(self.root / "packaging/legacy.sh").write_text("#!/bin/sh\necho $name\n")
		(self.root / "scripts/old.sh").write_text("#!/bin/sh\necho $name\n")
		(self.root / "subsurface.debug").write_text("#!/bin/sh\necho $name\n")
		self.git("add", "-A")
		self.git("commit", "-qm", "base")
		self.base = self.git("rev-parse", "HEAD").stdout.strip()

	def git(self, *args):
		return subprocess.run(("git", *args), cwd=self.root, capture_output=True, text=True, check=True)

	def report(self, base=None, head=None, actions=False):
		env = dict(os.environ, PR_BASE_SHA=base or self.base, PR_HEAD_SHA=head or self.git("rev-parse", "HEAD").stdout.strip())
		env["GITHUB_ACTIONS"] = "true" if actions else "false"
		if actions:
			env["GITHUB_STEP_SUMMARY"] = str(self.root / "summary.md")
		else:
			env.pop("GITHUB_STEP_SUMMARY", None)
		return subprocess.run(("python3", "scripts/shellcheck/report-pr.py"), cwd=self.root,
		                      env=env, capture_output=True, text=True)

	def test_changed_files_renames_deletions_and_existing_findings(self):
		# Old findings in an edited script are informational, not a CI failure.
		(self.root / "packaging/legacy.sh").write_text("#!/bin/sh\n# Existing warning\necho $name\n")
		(self.root / "subsurface.debug").write_text("#!/bin/sh\n# Existing warning\necho $name\n")
		self.git("mv", "scripts/old.sh", "scripts/renamed.sh")
		self.git("rm", "scripts/delete.sh")
		(self.root / "scripts/new name.sh").write_text("#!/bin/sh\necho $name\n")
		(self.root / "scripts/new\nline.sh").write_text("#!/bin/sh\necho $name\n")
		(self.root / "icons/clean.sh").write_text("#!/bin/sh\necho $name\n")
		(self.root / "packaging/untouched.sh").write_text("#!/bin/sh\necho $name\n")
		(self.root / "scripts/link.sh").symlink_to("../packaging/legacy.sh")
		self.git("add", "-A")
		self.git("commit", "-qm", "PR")
		# An untracked file is not in the PR and must not be reported.
		(self.root / "scripts/untracked.sh").write_text("#!/bin/sh\necho $name\n")
		result = self.report()
		self.assertEqual(result.returncode, 0, result.stderr)
		self.assertIn("Changed exempt shell scripts: 3", result.stdout)
		for name in ("packaging/legacy.sh", "scripts/renamed.sh", "subsurface.debug"):
			self.assertIn(name, result.stdout)
		for name in ("icons/clean.sh", "scripts/delete.sh", "scripts/untracked.sh", "scripts/old.sh",
		             "scripts/new name.sh", "scripts/new\nline.sh", "packaging/untouched.sh"):
			self.assertNotIn(repr(name) + ":", result.stdout)
		self.assertNotIn(repr("scripts/link.sh") + ":", result.stdout)
		self.assertIn("SC2154", result.stdout)
		self.assertIn("By directory: icons=0, packaging=1, scripts=1, other=1", result.stdout)
		self.assertIn("Total: 3 findings (informational, not a CI failure)", result.stdout)

	def test_no_changes_and_invalid_sha(self):
		result = self.report()
		self.assertEqual(result.returncode, 0, result.stderr)
		self.assertIn("Changed exempt shell scripts: 0", result.stdout)
		self.assertNotIn("By directory:", result.stdout)
		self.assertNotEqual(self.report(base="not a sha").returncode, 0)
		self.assertNotEqual(self.report(base="0" * 40).returncode, 0)

	def test_base_branch_changes_after_divergence_are_not_pr_changes(self):
		self.git("checkout", "-qb", "topic")
		(self.root / "scripts/added.sh").write_text("#!/bin/sh\necho $name\n")
		self.git("add", "scripts/added.sh")
		self.git("commit", "-qm", "PR change")
		head = self.git("rev-parse", "HEAD").stdout.strip()

		self.git("checkout", "-q", "master")
		(self.root / "packaging/legacy.sh").write_text("#!/bin/sh\n# Base-only edit\necho $name\n")
		self.git("add", "packaging/legacy.sh")
		self.git("commit", "-qm", "base change after divergence")
		base = self.git("rev-parse", "HEAD").stdout.strip()
		self.git("checkout", "-q", "topic")
		result = self.report(base=base, head=head)
		self.assertEqual(result.returncode, 0, result.stderr)
		self.assertIn("Changed exempt shell scripts: 1", result.stdout)
		self.assertIn("scripts/added.sh", result.stdout)
		self.assertNotIn(repr("packaging/legacy.sh") + ":", result.stdout)

	def test_enforced_set_fails_on_a_finding(self):
		clean = self.root / "icons/clean.sh"
		result = subprocess.run(("bash", "scripts/shellcheck/run.sh", "--check"),
		                        cwd=self.root, capture_output=True, text=True)
		self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
		clean.write_text("#!/bin/sh\necho $name\n")
		result = subprocess.run(("bash", "scripts/shellcheck/run.sh", "--check"),
		                        cwd=self.root, capture_output=True, text=True)
		self.assertEqual(result.returncode, 1)
		self.assertIn("SC2154", result.stdout)

	def test_new_script_is_enforced_by_default(self):
		new = self.root / "tools/new.sh"
		new.parent.mkdir()
		new.write_text("#!/bin/sh\necho hello\n")
		self.git("add", "tools/new.sh")
		result = subprocess.run(("bash", "scripts/shellcheck/run.sh", "--check"),
		                        cwd=self.root, capture_output=True, text=True)
		self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
		new.write_text("#!/bin/sh\necho $name\n")
		root_script = self.root / "new.sh"
		root_script.write_text("#!/bin/sh\necho $name\n")
		self.git("add", "new.sh")
		result = subprocess.run(("bash", "scripts/shellcheck/run.sh", "--check"),
		                        cwd=self.root, capture_output=True, text=True)
		self.assertEqual(result.returncode, 1)
		self.assertIn("SC2154", result.stdout)
		self.assertIn("In tools/new.sh line", result.stdout)
		self.assertIn("In new.sh line", result.stdout)

	def test_root_script_with_leading_hyphen_is_not_an_option(self):
		(self.root / "-new.sh").write_text("#!/bin/sh\necho $name\n")
		self.git("add", "--", "-new.sh")
		for mode in ("--check", "--report"):
			with self.subTest(mode=mode):
				result = subprocess.run(("bash", "scripts/shellcheck/run.sh", mode),
				                        cwd=self.root, capture_output=True, text=True)
				self.assertEqual(result.returncode, 1 if mode == "--check" else 0,
				                 result.stdout + result.stderr)
				self.assertIn("-new.sh", result.stdout)
				self.assertIn("SC2154", result.stdout)

	def test_runner_handles_newlines_and_unterminated_exemptions(self):
		name = "scripts/new\nline.sh"
		(self.root / name).write_text("#!/bin/sh\necho hello\n")
		self.git("add", "--", name)
		exempt = self.root / "scripts/shellcheck/exempt.txt"
		exempt.write_text(exempt.read_text().rstrip("\n"))
		result = subprocess.run(("bash", "scripts/shellcheck/run.sh", "--check"),
		                        cwd=self.root, capture_output=True, text=True)
		self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
		(self.root / name).write_text("#!/bin/sh\necho $name\n")
		result = subprocess.run(("bash", "scripts/shellcheck/run.sh", "--report"),
		                        cwd=self.root, capture_output=True, text=True)
		self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
		self.assertIn(name, result.stdout)
		self.assertIn("SC2154", result.stdout)

	def test_extensionless_script_is_covered(self):
		(self.root / "scripts/shellcheck/exempt.txt").write_text("packaging/legacy.sh\nscripts/old.sh\n")
		result = subprocess.run(("bash", "scripts/shellcheck/run.sh", "--check"),
		                        cwd=self.root, capture_output=True, text=True)
		self.assertEqual(result.returncode, 1)
		self.assertIn("In subsurface.debug line", result.stdout)

	def test_tracked_script_symlink_is_rejected(self):
		(self.root / "scripts/link.sh").symlink_to("../packaging/legacy.sh")
		self.git("add", "scripts/link.sh")
		result = subprocess.run(("bash", "scripts/shellcheck/run.sh", "--check"),
		                        cwd=self.root, capture_output=True, text=True)
		self.assertEqual(result.returncode, 2)
		self.assertIn("Not a regular shell script: scripts/link.sh", result.stderr)

	def test_git_enumeration_failure_fails_both_modes(self):
		fakebin = self.root / "fakebin"
		fakebin.mkdir()
		git = fakebin / "git"
		git.write_text("#!/bin/sh\necho 'git enumeration failed' >&2\nexit 7\n")
		git.chmod(0o755)
		env = dict(os.environ, PATH=str(fakebin) + os.pathsep + os.environ["PATH"])
		for mode in ("--check", "--report"):
			with self.subTest(mode=mode):
				result = subprocess.run(("bash", "scripts/shellcheck/run.sh", mode),
				                        cwd=self.root, env=env, capture_output=True, text=True)
				self.assertEqual(result.returncode, 7)
				self.assertIn("git enumeration failed", result.stderr)
				self.assertNotIn("SC", result.stdout)

	def test_github_report_is_informational(self):
		(self.root / "packaging/legacy.sh").write_text("#!/bin/sh\n# Changed\necho $name\n")
		self.git("add", "packaging/legacy.sh")
		self.git("commit", "-qm", "PR")
		result = self.report(actions=True)
		self.assertEqual(result.returncode, 0, result.stderr)
		self.assertIn("::warning file=packaging/legacy.sh,line=3,title=ShellCheck SC2154 (advisory)::", result.stdout)
		summary = (self.root / "summary.md").read_text()
		self.assertIn("1 existing finding", summary)
		self.assertIn("SC2154", summary)

	def test_annotation_fields_and_markdown_escape_untrusted_data(self):
		path = "packaging/a,%:\r\n# forged heading <script>.sh"
		message = "100%\r\n::error::injected </code><script>alert(1)</script>"
		issues = {path: [{"line": 7, "code": 2164, "level": "warning", "message": message}]}
		output = io.StringIO()
		with contextlib.redirect_stdout(output):
			shellcheck_report.emit_annotations(issues)
		annotation = output.getvalue()
		self.assertEqual(annotation.count("\n"), 1)
		self.assertIn("file=packaging/a%2C%25%3A%0D%0A# forged heading <script>.sh,line=7", annotation)
		self.assertIn("::100%25%0D%0A::error::injected", annotation)
		self.assertFalse(annotation.startswith("::error"))
		summary = shellcheck_report.summary_text(issues)
		self.assertIn("\\x0d\\x0a# forged heading &lt;script&gt;", summary)
		self.assertIn("100%\\x0d\\x0a::error::injected &lt;/code&gt;&lt;script&gt;", summary)
		self.assertNotIn("\n# forged heading", summary)

	def test_annotation_cap_and_summary_size_limit(self):
		issues = {"scripts/legacy.sh": [{"line": line, "code": 2086, "message": "Needs quoting"}
		                                for line in range(1, 13)]}
		output = io.StringIO()
		with contextlib.redirect_stdout(output):
			shellcheck_report.emit_annotations(issues)
		self.assertEqual(output.getvalue().count("::warning file="), 10)
		full = shellcheck_report.summary_text(issues)
		self.assertEqual(full.count("<code>scripts/legacy.sh:"), 12)
		self.assertIn("12 findings (SC2086=12)", full)
		large = {"scripts/legacy.sh": [{"line": line, "code": 2086, "message": "x" * 1000}
		                                for line in range(1, 13)]}
		limited = shellcheck_report.summary_text(large, limit=1800)
		self.assertLessEqual(len(limited.encode("utf-8")), 1800)
		self.assertIn("12 findings (SC2086=12)", limited)
		self.assertIn("further findings omitted from this summary", limited)

	def test_summary_omits_per_file_headers_when_they_exceed_limit(self):
		issues = {f"packaging/{i:04d}-legacy.sh": [{"line": 2, "code": 2086, "message": "Needs quoting"}]
		          for i in range(100)}
		summary = shellcheck_report.summary_text(issues, limit=600)
		self.assertLessEqual(len(summary.encode("utf-8")), 600)
		self.assertIn("100 existing findings in 100 changed exempt scripts", summary)
		self.assertIn("By directory: icons=0, packaging=100, scripts=0, other=0", summary)
		self.assertIn("Per-file counts and 100 findings omitted", summary)
		self.assertNotIn("packaging/0000-legacy.sh", summary)


if __name__ == "__main__":
	unittest.main()
