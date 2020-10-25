// SPDX-License-Identifier: GPL-2.0
#ifndef SUBSURFACESTARTUP_H
#define SUBSURFACESTARTUP_H

#ifdef __cplusplus
extern "C" {
#else
#include <stdbool.h>
#endif

extern bool imported;
extern int quit, force_root, ignore_bt;
#ifdef SUBSURFACE_MOBILE_DESKTOP
extern char *testqml;
#endif

void setup_system_prefs(void);
void parse_argument(const char *arg);
void free_prefs(void);
void print_files(void);
void print_version(void);

extern char *settings_suffix;

#ifdef __cplusplus
}
#endif

#endif // SUBSURFACESTARTUP_H
