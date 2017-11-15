// SPDX-License-Identifier: GPL-2.0
#ifndef SUBSURFACESTARTUP_H
#define SUBSURFACESTARTUP_H

#include "dive.h"
#include "divelist.h"
#include "libdivecomputer.h"

#ifdef __cplusplus
extern "C" {
#else
#include <stdbool.h>
#endif

extern bool imported;

void setup_system_prefs(void);
void parse_argument(const char *arg);
void free_prefs(void);
void copy_prefs(struct preferences *src, struct preferences *dest);
void print_files(void);
void print_version(void);

extern char *settings_suffix;

#ifdef __cplusplus
}
#endif

#endif // SUBSURFACESTARTUP_H
