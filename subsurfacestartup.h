#ifndef SUBSURFACESTARTUP_H
#define SUBSURFACESTARTUP_H

#include "dive.h"
#include "divelist.h"
#include "libdivecomputer.h"
#include "version.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

extern struct preferences prefs;
extern struct preferences default_prefs;
extern bool imported;

void setup_system_prefs(void);
void parse_argument(const char *arg);

#ifdef __cplusplus
}
#endif

#endif
