// SPDX-License-Identifier: GPL-2.0
#include "version.h"
#include "ssrf-version.h"

#include <cstring>

// let's leave the two redundant functions in case we change our minds on git SHAs
const char *subsurface_git_version()
{
	return CANONICAL_VERSION_STRING_4;
}

const char *subsurface_canonical_version()
{
	return CANONICAL_VERSION_STRING;
}

// AI-generated (Claude)
// Return the extension of the version string, that is everything after the
// first '-' without an optional leading "patch.<N>." component, or nullptr if
// there is none.
static const char *version_extension(const char *version)
{
	if (!version)
		return nullptr;
	const char *ext = strchr(version, '-');
	if (!ext)
		return nullptr;
	ext++;
	if (strncmp(ext, "patch.", 6) == 0) {
		const char *p = ext + 6;
		const char *digits = p;
		while (*p >= '0' && *p <= '9')
			p++;
		if (p != digits && *p == '.')
			ext = p + 1;
	}
	return ext;
}

static bool starts_with(const char *ext, const char *prefix)
{
	return strncmp(ext, prefix, strlen(prefix)) == 0;
}

build_kind parse_build_kind(const char *version)
{
	const char *ext = version_extension(version);
	if (!ext)
		return build_kind::local;
	if (strcmp(ext, "CICD-release") == 0)
		return build_kind::ci;
	if (strcmp(ext, "pull-request") == 0 || starts_with(ext, "pull-request.") ||
	    strcmp(ext, "CICD-pull-request") == 0 || strcmp(ext, "adhoc") == 0)
		return build_kind::pr;
	return build_kind::local;
}

int parse_pr_number(const char *version)
{
	const char *ext = version_extension(version);
	if (!ext || !starts_with(ext, "pull-request."))
		return 0;
	const char *digits = ext + strlen("pull-request.");
	// only the pure number form counts, a branch name containing digits does not
	// (nine digits keeps the value well inside an int)
	int number = 0;
	int count = 0;
	for (const char *p = digits; *p; p++) {
		if (*p < '0' || *p > '9' || ++count > 9)
			return 0;
		number = number * 10 + (*p - '0');
	}
	return number;
}

build_kind subsurface_build_kind()
{
	return parse_build_kind(CANONICAL_VERSION_STRING);
}

int subsurface_pr_number()
{
	return parse_pr_number(CANONICAL_VERSION_STRING);
}

static int min_datafile_version = 0;

int get_min_datafile_version()
{
	return min_datafile_version;
}

void report_datafile_version(int version)
{
	if (min_datafile_version == 0 || min_datafile_version > version)
		min_datafile_version = version;
}

void clear_min_datafile_version()
{
	min_datafile_version = 0;
}
