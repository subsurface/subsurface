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
