// SPDX-License-Identifier: GPL-2.0
#ifndef PICTURE_H
#define PICTURE_H

// picture (more precisely media) related strutures and functions
#include "units.h"

struct picture {
	char *filename;
	offset_t offset;
	location_t location;
	struct picture *next;
};

extern struct picture *alloc_picture();
extern void free_picture(struct picture *picture);

#endif // PICTURE_H
