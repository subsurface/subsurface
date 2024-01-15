// SPDX-License-Identifier: GPL-2.0
#ifndef PICTURE_H
#define PICTURE_H

// picture (more precisely media) related strutures and functions
#include "units.h"
#include <stddef.h> // For NULL

#ifdef __cplusplus
extern "C" {
#endif

struct dive;

struct picture {
	char *filename;
	offset_t offset;
	location_t location;
};
static const struct picture empty_picture = { NULL, { 0 }, { { 0 }, { 0 } } };

/* loop through all pictures of a dive */
#define FOR_EACH_PICTURE(_dive)								\
	if ((_dive) && (_dive)->pictures.nr)						\
		for (struct picture *picture = (_dive)->pictures.pictures;		\
		     picture < (_dive)->pictures.pictures + (_dive)->pictures.nr;	\
		     picture++)

/* Table of pictures. Attention: this stores pictures,
 * *not* pointers to pictures. This has two crucial consequences:
 * 1) Pointers to pictures are not stable. They may be
 *    invalidated if the table is reallocated.
 * 2) add_to_picture_table(), etc. take ownership of the
 *    picture. Notably of the filename. */
struct picture_table {
	int nr, allocated;
	struct picture *pictures;
};

/* picture table functions */
extern void clear_picture_table(struct picture_table *);
extern void add_to_picture_table(struct picture_table *, int idx, struct picture pic);
extern void copy_pictures(const struct picture_table *s, struct picture_table *d);
extern void add_picture(struct picture_table *, struct picture newpic);
extern void remove_from_picture_table(struct picture_table *, int idx);
extern int get_picture_idx(const struct picture_table *, const char *filename); /* Return -1 if not found */
extern void sort_picture_table(struct picture_table *);

extern struct picture *create_picture(const char *filename, timestamp_t shift_time, bool match_all, struct dive **dive);
extern bool picture_check_valid_time(timestamp_t timestamp, timestamp_t shift_time);

#ifdef __cplusplus
}
#endif

#endif // PICTURE_H
