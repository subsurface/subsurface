#!/usr/bin/env python3
"""Report, but do not fail on, ShellCheck findings in changed exempt scripts."""

import json
import html
import os
from pathlib import Path
import re
import subprocess
from collections import Counter


ROOT = Path(__file__).resolve().parents[2]
DIRECTORIES = ("icons", "packaging", "scripts", "other")
ANNOTATION_LIMIT = 10
SUMMARY_LIMIT = 256 * 1024
TRUNCATION_RESERVE = 256


def markdown_text(value):
	# Keep file names and diagnostic text inside one Markdown/HTML line.
	value = "".join(f"\\x{ord(char):02x}" if ord(char) < 32 or ord(char) == 127 else char
	                for char in str(value))
	return html.escape(value)


def escape_command(value, property=False):
	value = str(value).replace("%", "%25").replace("\r", "%0D").replace("\n", "%0A")
	if property:
		value = value.replace(":", "%3A").replace(",", "%2C")
	return value


def emit_annotations(by_file):
	count = 0
	for path, issues in by_file.items():
		for issue in issues:
			if count == ANNOTATION_LIMIT:
				return
			code = int(issue["code"])
			line = int(issue["line"])
			print(f"::warning file={escape_command(path, property=True)},line={line},"
			      f"title=ShellCheck SC{code} (advisory)::{escape_command(issue['message'])}")
			count += 1


def summary_text(by_file, limit=SUMMARY_LIMIT):
	total = sum(len(issues) for issues in by_file.values())
	counts = Counter({directory: 0 for directory in DIRECTORIES})
	lines = ["## Advisory ShellCheck findings\n\n",
	         f"{total} existing {'finding' if total == 1 else 'findings'} "
	         f"in {len(by_file)} changed exempt scripts; "
	         "the check remains non-blocking.\n\n"]
	for path, issues in by_file.items():
		directory = path.split("/", 1)[0]
		counts[directory if directory in DIRECTORIES else "other"] += len(issues)
		codes = Counter(f"SC{int(issue['code'])}" for issue in issues)
		code_list = ", ".join(f"{code}={count}" for code, count in sorted(codes.items())) or "none"
		lines.append(f"- <code>{markdown_text(path)}</code>: {len(issues)} "
		             f"{'finding' if len(issues) == 1 else 'findings'} ({code_list})\n")
	lines.append("\nBy directory: " + ", ".join(f"{name}={counts[name]}" for name in DIRECTORIES) + "\n\n")
	lines.append("### Findings\n\n")
	text = "".join(lines)
	# Reserve space for the truncation notice, even when all details fit.
	if len(text.encode("utf-8")) + TRUNCATION_RESERVE > limit:
		fallback = ("".join(lines[:2]) + lines[-2] + lines[-1] +
		            f"Per-file counts and {total} findings omitted from this summary; "
		            "run scripts/shellcheck/run.sh --report locally.\n")
		if len(fallback.encode("utf-8")) > limit:
			raise ValueError("ShellCheck summary limit too small for totals")
		return fallback
	size = len(text.encode("utf-8"))
	omitted = 0
	for path, issues in by_file.items():
		for issue in issues:
			entry = (f"- <code>{markdown_text(path)}:{int(issue['line'])}</code> "
			         f"<code>SC{int(issue['code'])}</code>: <code>{markdown_text(issue['message'])}</code>\n")
			entry_size = len(entry.encode("utf-8"))
			if size + entry_size + TRUNCATION_RESERVE <= limit:
				lines.append(entry)
				size += entry_size
			else:
				omitted += 1
	if omitted:
		lines.append(f"\n{omitted} further findings omitted from this summary; "
		             "run scripts/shellcheck/run.sh --report locally.\n")
	return "".join(lines)


def changed_scripts(base, head):
	for sha in (base, head):
		if not re.fullmatch(r"[0-9a-fA-F]{40}", sha):
			raise ValueError("PR base and head must be full commit SHAs")
	merge_base = subprocess.run(
		["git", "merge-base", base, head], cwd=ROOT, capture_output=True, text=True, check=True
	).stdout.strip()
	result = subprocess.run(
		["git", "diff", "--name-only", "-z", "--diff-filter=ACMRT", merge_base, head],
		cwd=ROOT, capture_output=True, check=True
	)
	for raw in result.stdout.split(b"\0"):
		if not raw:
			continue
		path = os.fsdecode(raw)
		file = ROOT / path
		if (path.endswith(".sh") or path == "subsurface.debug") and file.is_file() and not file.is_symlink():
			yield path


def main():
	exempt = set((ROOT / "scripts/shellcheck/exempt.txt").read_text().splitlines())
	paths = sorted(set(changed_scripts(os.environ["PR_BASE_SHA"], os.environ["PR_HEAD_SHA"])) & exempt)
	print(f"Changed exempt shell scripts: {len(paths)}", flush=True)
	if not paths:
		return

	result = subprocess.run(
		["shellcheck", "--severity=warning", "--format=json", "--", *("./" + p for p in paths)],
		cwd=ROOT, capture_output=True, text=True
	)
	if result.returncode not in (0, 1):
		raise RuntimeError(f"ShellCheck failed (exit {result.returncode}): {result.stderr}")
	findings = json.loads(result.stdout)
	by_file = {path: [] for path in paths}
	for finding in findings:
		path = finding["file"].removeprefix("./")
		if path not in by_file:
			raise ValueError(f"Unexpected ShellCheck path: {path!r}")
		by_file[path].append(finding)

	counts = Counter({directory: 0 for directory in DIRECTORIES})
	for path, issues in by_file.items():
		directory = path.split("/", 1)[0]
		counts[directory if directory in DIRECTORIES else "other"] += len(issues)
		codes = Counter(f"SC{issue['code']}" for issue in issues)
		print(f"  {path!r}: {len(issues)} findings; " +
		      (", ".join(f"{code}={count}" for code, count in sorted(codes.items())) or "none"))
		for issue in issues[:10]:
			message = issue["message"].replace("\n", " ").replace("\r", " ")
			print(f"    line {issue['line']}: {issue['level']} SC{issue['code']}: {message}")
		if len(issues) > 10:
			print(f"    ... {len(issues) - 10} more findings; run scripts/shellcheck/run.sh --report locally")
	print("By directory: " + ", ".join(f"{name}={counts[name]}" for name in DIRECTORIES))
	print(f"Total: {len(findings)} findings (informational, not a CI failure)")
	if os.environ.get("GITHUB_ACTIONS") == "true":
		emit_annotations(by_file)
	if os.environ.get("GITHUB_STEP_SUMMARY"):
		with open(os.environ["GITHUB_STEP_SUMMARY"], "a", encoding="utf-8") as summary:
			summary.write(summary_text(by_file))


if __name__ == "__main__":
	main()
