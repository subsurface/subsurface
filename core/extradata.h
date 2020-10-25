// SPDX-License-Identifier: GPL-2.0
#ifndef EXTRADATA_H
#define EXTRADATA_H

struct extra_data {
	const char *key;
	const char *value;
	struct extra_data *next;
};

#endif
