// SPDX-License-Identifier: GPL-2.0
#ifndef SUBSURFACESTARTUP_H
#define SUBSURFACESTARTUP_H

extern "C" {

extern bool imported;
extern int quit, force_root, ignore_bt;

void setup_system_prefs(void);
void parse_argument(const char *arg);
void free_prefs(void);
void print_files(void);
void print_version(void);

extern char *settings_suffix;

}

#ifdef SUBSURFACE_MOBILE_DESKTOP
#include <string>
extern std::string testqml;
#endif

#endif // SUBSURFACESTARTUP_H
