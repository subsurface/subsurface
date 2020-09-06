// SPDX-License-Identifier: GPL-2.0
#include "taxonomy.h"
#include "gettext.h"
#include "subsurface-string.h"
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

static void alloc_taxonomy_table(struct taxonomy_data *t)
{
	if (!t->category)
		t->category = calloc(TC_NR_CATEGORIES, sizeof(struct taxonomy));
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

void copy_taxonomy(const struct taxonomy_data *orig, struct taxonomy_data *copy)
{
	if (orig->category == NULL) {
		free_taxonomy(copy);
	} else {
		alloc_taxonomy_table(copy);
		for (int i = 0; i < TC_NR_CATEGORIES; i++) {
			if (i < copy->nr) {
				free((void *)copy->category[i].value);
				copy->category[i].value = NULL;
			}
			if (i < orig->nr) {
				copy->category[i] = orig->category[i];
				copy->category[i].value = copy_string(orig->category[i].value);
			}
		}
		copy->nr = orig->nr;
	}
}

static int taxonomy_index_for_category(const struct taxonomy_data *t, enum taxonomy_category cat)
{
	for (int i = 0; i < t->nr; i++) {
		if (t->category[i].category == cat)
			return i;
	}
	return -1;
}

const char *taxonomy_get_value(const struct taxonomy_data *t, enum taxonomy_category cat)
{
	int idx = taxonomy_index_for_category(t, cat);
	return idx >= 0 ? t->category[idx].value : NULL;
}

const char *taxonomy_get_country(const struct taxonomy_data *t)
{
	return taxonomy_get_value(t, TC_COUNTRY);
}

void taxonomy_set_category(struct taxonomy_data *t, enum taxonomy_category category, const char *value, enum taxonomy_origin origin)
{
	int idx = taxonomy_index_for_category(t, category);

	if (idx < 0) {
		alloc_taxonomy_table(t); // make sure we have taxonomy data allocated
		if (t->nr == TC_NR_CATEGORIES - 1) {
			// can't add another one
			fprintf(stderr, "Error adding taxonomy category\n");
			return;
		}
		idx = t->nr++;
	} else {
		free((void *)t->category[idx].value);
		t->category[idx].value = NULL;
	}
	t->category[idx].value = strdup(value);
	t->category[idx].origin = origin;
	t->category[idx].category = category;
}

void taxonomy_set_country(struct taxonomy_data *t, const char *country, enum taxonomy_origin origin)
{
	fprintf(stderr, "%s: set the taxonomy country to %s\n", __func__, country);
	taxonomy_set_category(t, TC_COUNTRY, country, origin);
}
