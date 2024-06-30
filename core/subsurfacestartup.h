// SPDX-License-Identifier: GPL-2.0
#ifndef SUBSURFACESTARTUP_H
#define SUBSURFACESTARTUP_H

#include <string>

extern bool imported;
extern int quit, force_root, ignore_bt;

void setup_system_prefs();
void parse_argument(const char *arg);
void print_files();
void print_version();

void subsurface_console_init();
void subsurface_console_exit();
bool subsurface_user_is_root();

extern std::string settings_suffix;

#ifdef SUBSURFACE_MOBILE_DESKTOP
extern std::string testqml;
#endif

#endif // SUBSURFACESTARTUP_H
