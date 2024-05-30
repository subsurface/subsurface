// SPDX-License-Identifier: GPL-2.0
// picture (more precisely media) related strutures and functions
#ifndef PICTURE_H
#define PICTURE_H

#include "units.h"
#include <optional>
#include <string>
#include <vector>

struct dive;

struct picture {
	std::string filename;
	offset_t offset;
	location_t location;
	bool operator<(const picture &) const;
};

/* Table of pictures. Attention: this stores pictures,
 * *not* pointers to pictures. This means that
 * pointers to pictures are not stable. They are
 * invalidated if the table is reallocated.
 */
using picture_table = std::vector<picture>;

/* picture table functions */
extern void add_to_picture_table(picture_table &, int idx, struct picture pic);
extern void add_picture(picture_table &, struct picture newpic);
extern int get_picture_idx(const picture_table &, const std::string &filename); /* Return -1 if not found */

extern std::pair<std::optional<picture>, dive *> create_picture(const std::string &filename, timestamp_t shift_time, bool match_all);
extern bool picture_check_valid_time(timestamp_t timestamp, timestamp_t shift_time);

#endif // PICTURE_H
