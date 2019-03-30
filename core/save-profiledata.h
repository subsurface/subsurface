// SPDX-License-Identifier: GPL-2.0
#ifndef SAVE_PROFILE_DATA_H
#define SAVE_PROFILE_DATA_H

#include "dive.h"

#ifdef __cplusplus
extern "C" {
#endif

int save_profiledata(const char *filename, bool selected_only);

#ifdef __cplusplus
}
#endif
#endif // SAVE_PROFILE_DATA_H
