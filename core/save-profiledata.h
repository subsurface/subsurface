// SPDX-License-Identifier: GPL-2.0
#ifndef SAVE_PROFILE_DATA_H
#define SAVE_PROFILE_DATA_H

int save_profiledata(const char *filename, bool selected_only);
void save_subtitles_buffer(struct membuffer *b, struct dive *dive, int offset, int length);

#endif // SAVE_PROFILE_DATA_H
