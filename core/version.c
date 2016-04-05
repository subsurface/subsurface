#include "ssrf-version.h"

const char *subsurface_git_version(void)
{
	return GIT_VERSION_STRING;
}

const char *subsurface_canonical_version(void)
{
	return CANONICAL_VERSION_STRING;
}

#ifdef SUBSURFACE_MOBILE
const char *subsurface_mobile_version(void)
{
	return MOBILE_VERSION_STRING;
}
#endif
