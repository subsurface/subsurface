// SPDX-License-Identifier: GPL-2.0
#include "qPrefGeocoding.h"
#include "qPrefPrivate.h"

static const QString group = QStringLiteral("geocoding");

qPrefGeocoding *qPrefGeocoding::instance()
{
	static qPrefGeocoding *self = new qPrefGeocoding;
	return self;
}

void qPrefGeocoding::loadSync(bool doSync)
{
	disk_first_taxonomy_category(doSync);
	disk_second_taxonomy_category(doSync);
	disk_third_taxonomy_category(doSync);
}

void qPrefGeocoding::set_first_taxonomy_category(taxonomy_category value)
{
	if (value != prefs.geocoding.category[0]) {
		prefs.geocoding.category[0] = value;
		disk_first_taxonomy_category(true);
		emit instance()->first_taxonomy_categoryChanged(value);
	}
}
void qPrefGeocoding::disk_first_taxonomy_category(bool doSync)
{
	if (doSync)
		qPrefPrivate::propSetValue(keyFromGroupAndName(group, "cat0"), prefs.geocoding.category[0], default_prefs.geocoding.category[0]);
	else
		prefs.geocoding.category[0] = (enum taxonomy_category)qPrefPrivate::propValue(keyFromGroupAndName(group, "cat0"), default_prefs.geocoding.category[0]).toInt();
}

void qPrefGeocoding::set_second_taxonomy_category(taxonomy_category value)
{
	if (value != prefs.geocoding.category[1]) {
		prefs.geocoding.category[1] = value;
		disk_second_taxonomy_category(true);
		emit instance()->second_taxonomy_categoryChanged(value);
	}
}

void qPrefGeocoding::disk_second_taxonomy_category(bool doSync)
{
	if (doSync)
		qPrefPrivate::propSetValue(keyFromGroupAndName(group, "cat1"), prefs.geocoding.category[1], default_prefs.geocoding.category[1]);
	else
		prefs.geocoding.category[1] = (enum taxonomy_category)qPrefPrivate::propValue(keyFromGroupAndName(group, "cat1"), default_prefs.geocoding.category[1]).toInt();
}


void qPrefGeocoding::set_third_taxonomy_category(taxonomy_category value)
{
	if (value != prefs.geocoding.category[2]) {
		prefs.geocoding.category[2] = value;
		disk_third_taxonomy_category(true);
		emit instance()->third_taxonomy_categoryChanged(value);
	}
}

void qPrefGeocoding::disk_third_taxonomy_category(bool doSync)
{
	if (doSync)
		qPrefPrivate::propSetValue(keyFromGroupAndName(group, "cat2"), prefs.geocoding.category[2], default_prefs.geocoding.category[2]);
	else
		prefs.geocoding.category[2] = (enum taxonomy_category)qPrefPrivate::propValue(keyFromGroupAndName(group, "cat2"), default_prefs.geocoding.category[2]).toInt();
}
