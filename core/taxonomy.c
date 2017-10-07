// SPDX-License-Identifier: GPL-2.0
#include "taxonomy.h"
#include "gettext.h"
#include <stdlib.h>
#include <stdio.h>

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

const char *taxonomy_get_country(struct taxonomy_data *t)
{
	for (int i = 0; i < t->nr; i++) {
		if (t->category[i].category == TC_COUNTRY)
			return t->category[i].value;
	}
	return NULL;
}

void taxonomy_set_country(struct taxonomy_data *t, const char *country, enum taxonomy_origin origin)
{
	int idx = -1;

	// make sure we have taxonomy data allocated
	if (!t->category)
		t->category = alloc_taxonomy();

	for (int i = 0; i < t->nr; i++) {
		if (t->category[i].category == TC_COUNTRY) {
			idx = i;
			break;
		}
	}
	if (idx == -1) {
		if (t->nr == TC_NR_CATEGORIES - 1) {
			// can't add another one
			fprintf(stderr, "Error adding country taxonomy\n");
			return;
		}
		idx = t->nr++;
	}
	t->category[idx].value = country;
	t->category[idx].origin = origin;
	t->category[idx].category = TC_COUNTRY;
	fprintf(stderr, "%s: set the taxonomy country to %s\n", __func__, country);
}
