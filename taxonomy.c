#include "taxonomy.h"
#include "gettext.h"
#include <stdlib.h>

char *taxonomy_category_names[TC_NR_CATEGORIES] = {
	QT_TRANSLATE_NOOP("getTextFromC", "None"),
	QT_TRANSLATE_NOOP("getTextFromC", "Ocean"),
	QT_TRANSLATE_NOOP("getTextFromC", "Country"),
	QT_TRANSLATE_NOOP("getTextFromC", "State"),
	QT_TRANSLATE_NOOP("getTextFromC", "County"),
	QT_TRANSLATE_NOOP("getTextFromC", "City")
};

// these are the names for geoname.org
char *taxonomy_api_names[TC_NR_CATEGORIES] = {
	"none",
	"name",
	"countryName",
	"adminName1",
	"adminName2",
	"toponymName"
};

struct taxonomy *alloc_taxonomy()
{
	return calloc(TC_NR_CATEGORIES, sizeof(struct taxonomy));
}

void free_taxonomy(struct taxonomy *t)
{
	if (t) {
		for (int i = 0; i < TC_NR_CATEGORIES; i++)
			free((void *)t[i].value);
		free(t);
	}
}
