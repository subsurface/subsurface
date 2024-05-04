// SPDX-License-Identifier: GPL-2.0
#include "taxonomy.h"
#include "errorhelper.h"
#include "gettext.h"
#include "subsurface-string.h"
#include <algorithm>
#include <stdlib.h>
#include <stdio.h>
#include <QtGlobal> // for QT_TRANSLATE_NOOP

const char *taxonomy_category_names[TC_NR_CATEGORIES] = {
	QT_TRANSLATE_NOOP("gettextFromC", "None"),
	QT_TRANSLATE_NOOP("gettextFromC", "Ocean"),
	QT_TRANSLATE_NOOP("gettextFromC", "Country"),
	QT_TRANSLATE_NOOP("gettextFromC", "State"),
	QT_TRANSLATE_NOOP("gettextFromC", "County"),
	QT_TRANSLATE_NOOP("gettextFromC", "Town"),
	QT_TRANSLATE_NOOP("gettextFromC", "City")
};

// these are the names for geoname.org
const char *taxonomy_api_names[TC_NR_CATEGORIES] = {
	"none",
	"name",
	"countryName",
	"adminName1",
	"adminName2",
	"toponymName",
	"adminName3"
};

std::string taxonomy_get_value(const taxonomy_data &t, enum taxonomy_category cat)
{
	auto it = std::find_if(t.begin(), t.end(), [cat] (const taxonomy &tax) { return tax.category == cat; });
	return it != t.end() ? it->value : std::string();
}

std::string taxonomy_get_country(const taxonomy_data &t)
{
	return taxonomy_get_value(t, TC_COUNTRY);
}

void taxonomy_set_category(taxonomy_data &t, enum taxonomy_category cat, const std::string &value, enum taxonomy_origin origin)
{
	auto it = std::find_if(t.begin(), t.end(), [cat] (const taxonomy &tax) { return tax.category == cat; });
	if (it == t.end()) {
		t.emplace_back();
		it = std::prev(t.end());
	}
	it->value = value;
	it->origin = origin;
	it->category = cat;
}

void taxonomy_set_country(taxonomy_data &t, const std::string &country, enum taxonomy_origin origin)
{
	report_info("%s: set the taxonomy country to %s\n", __func__, country.c_str());
	taxonomy_set_category(t, TC_COUNTRY, country, origin);
}
