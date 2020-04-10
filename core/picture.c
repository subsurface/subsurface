// SPDX-License-Identifier: GPL-2.0
#include "picture.h"
#include <stdlib.h>
#include <string.h>

struct picture *alloc_picture()
{
	struct picture *pic = malloc(sizeof(struct picture));
	if (!pic)
		exit(1);
	memset(pic, 0, sizeof(struct picture));
	return pic;
}

void free_picture(struct picture *picture)
{
	if (picture) {
		free(picture->filename);
		free(picture);
	}
}
