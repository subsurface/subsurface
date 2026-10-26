# ShellCheck in CI

`scripts/shellcheck/run.sh --check` discovers tracked `.sh` files throughout the
repository and `subsurface.debug`, and enforces ShellCheck at warning severity
or higher on every script **except** those in `scripts/shellcheck/exempt.txt`.
New `.sh` scripts are checked automatically, including in new directories.
The workflow also runs for changes to any `.sh` script or `subsurface.debug`.

The exemption list records scripts with existing findings. A changed exempt
script remains advisory: on pull requests, the **Report findings in changed
exempt scripts** step lists its diagnostic counts and codes, up to ten findings
per file in the logs, and totals by directory. On GitHub Actions it also creates
up to ten warning annotations per step and a job summary with every finding.
If the summary exceeds 256 KiB, it retains the counts and notes how many
details were omitted; `--report` can show the full diagnostics locally.
Findings do not fail this step or generate PR comments. The step selects changed
paths by comparing the PR head with its merge base against the base commit;
deleted files and symlinks are skipped.
All other scripts are checked by the blocking step whether or not they were
edited. Remove a script from the exemption list once its findings are fixed.

`packaging/macosx/sign.sh` is a local packaging helper checked by the blocking step.
`subsurface.debug` keeps its extensionless name because it is installed as a
command under that name; it is explicitly covered and exempted for its current
findings. Other extensionless shell scripts require an explicit coverage change.

Run `scripts/shellcheck/run.sh` locally for the blocking check and
`scripts/shellcheck/run.sh --report` for all tracked scripts, including exempt
ones. `--report` exits successfully on ShellCheck findings, but fails for tool
errors. Install ShellCheck locally first; CI installs version 0.11.0-2 on
Ubuntu 26.04.
CI also runs `python3 -B scripts/shellcheck/test-report-pr.py` to test the
enforced and advisory policies.
