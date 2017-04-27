// SPDX-License-Identifier: GPL-2.0
#include "taxonomy.h"
#include "gettext.h"
#include <stdlib.h>

char *taxonomy_category_names[TC_NR_CATEGORIES] = {
	QT_TRANSLATE_NOOP("gettextFromC", "None"),
	QT_TRANSLATE_NOOP("gettextFromC", "Ocean"),
	QT_TRANSLATE_NOOP("gettextFromC", "Country"),
	QT_TRANSLATE_NOOP("gettextFromC", "State"),
	QT_TRANSLATE_NOOP("gettextFromC", "County"),
	QT_TRANSLATE_NOOP("gettextFromC", "Town"),
	QT_TRANSLATE_NOOP("gettextFromC", "City")
};

// these are the names for geoname.org
char *taxonomy_api_names[TC_NR_CATEGORIES] = {
	"none",
	"name",
	"countryName",
	"adminName1",
	"adminName2",
	"toponymName",
	"adminName3"
};

struct taxonomy *alloc_taxonomy()
{
	return calloc(TC_NR_CATEGORIES, sizeof(struct taxonomy));
}

void free_taxonomy(struct taxonomy_data *t)
{
	if (t) {
		for (int i = 0; i < t->nr; i++)
			free((void *)t->category[i].value);
		free(t->category);
		t->category = NULL;
		t->nr = 0;
	}
}

int taxonomy_index_for_category(struct taxonomy_data *t, enum taxonomy_category cat)
{
	for (int i = 0; i < t->nr; i++)
		if (t->category[i].category == cat)
			return i;
	return -1;
}
