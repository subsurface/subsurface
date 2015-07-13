#include "taxonomy.h"
#include "gettext.h"
#include <stdlib.h>

char *taxonomy_category_names[TC_NR_CATEGORIES] = {
	QT_TRANSLATE_NOOP("getTextFromC", "None"),
	QT_TRANSLATE_NOOP("getTextFromC", "Ocean"),
	QT_TRANSLATE_NOOP("getTextFromC", "Country"),
	QT_TRANSLATE_NOOP("getTextFromC", "State"),
	QT_TRANSLATE_NOOP("getTextFromC", "County"),
	QT_TRANSLATE_NOOP("getTextFromC", "Town"),
	QT_TRANSLATE_NOOP("getTextFromC", "City")
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
