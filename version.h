#ifndef VERSION_H
#define VERSION_H

#ifdef __cplusplus
extern "C" {
#endif

const char *subsurface_version(void);
const char *subsurface_git_version(void);
const char *subsurface_canonical_version(void);

#ifdef __cplusplus
}
#endif

#endif
