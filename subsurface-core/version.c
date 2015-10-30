#include "ssrf-version.h"

const char *subsurface_version(void)
{
	return VERSION_STRING;
}

const char *subsurface_git_version(void)
{
	return GIT_VERSION_STRING;
}

const char *subsurface_canonical_version(void)
{
	return CANONICAL_VERSION_STRING;
}
