#ifndef VERSION_H
#define VERSION_H

/* this is used for both git and xml format */
static constexpr int dataformat_version = 3;

const char *subsurface_git_version();
const char *subsurface_canonical_version();

// AI-generated (Claude)
// The kind of build that is running, derived from the extension of the
// canonical version string (everything after the first '-'):
//   ci:    CICD-release, the latest (current) CICD build
//   pr:    pull-request.<number>, the legacy pull-request.<branch> and
//          pull-request., CICD-pull-request, and adhoc (manual CI run)
//   local: local, and any missing or unrecognised extension
enum class build_kind {
	local,
	pr,
	ci
};

// The version string is parsed as <base>[-patch.<N>.]<extension>. These take the
// version string as argument so that they can be tested; the two functions
// below apply them to the version of this binary.
build_kind parse_build_kind(const char *version);
// The pull request number, or 0 for a build that is not a pull request build
// or whose pull request number is unknown.
int parse_pr_number(const char *version);

build_kind subsurface_build_kind();
int subsurface_pr_number();
int get_min_datafile_version();
void report_datafile_version(int version);
void clear_min_datafile_version();

#endif
