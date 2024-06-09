// SPDX-License-Identifier: GPL-2.0
#ifndef SAVE_PROFILE_DATA_H
#define SAVE_PROFILE_DATA_H

#include <string>

int save_profiledata(const char *filename, bool selected_only);
std::string save_subtitles_buffer(struct dive *dive, int offset, int length);

#endif // SAVE_PROFILE_DATA_H
