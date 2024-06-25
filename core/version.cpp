// SPDX-License-Identifier: GPL-2.0
#include "ssrf-version.h"

// let's leave the two redundant functions in case we change our minds on git SHAs
const char *subsurface_git_version()
{
	return CANONICAL_VERSION_STRING_4;
}

const char *subsurface_canonical_version()
{
	return CANONICAL_VERSION_STRING;
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

int clear_min_datafile_version()
{
	return min_datafile_version;
}
