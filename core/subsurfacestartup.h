// SPDX-License-Identifier: GPL-2.0
#ifndef SUBSURFACESTARTUP_H
#define SUBSURFACESTARTUP_H

extern bool imported;
extern int quit, force_root, ignore_bt;

void setup_system_prefs();
void parse_argument(const char *arg);
void free_prefs();
void print_files();
void print_version();

extern char *settings_suffix;

#ifdef SUBSURFACE_MOBILE_DESKTOP
#include <string>
extern std::string testqml;
#endif

#endif // SUBSURFACESTARTUP_H
