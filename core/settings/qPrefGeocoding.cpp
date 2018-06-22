// SPDX-License-Identifier: GPL-2.0
#include "qPrefGeocoding.h"
#include "qPref_private.h"


/*
	void diskFirstTaxonomyCategory(bool doSync);
	void diskSecondTaxonomyCategory(bool doSync);
	void diskThirdTaxonomyCategory(bool doSync);
		*/

taxonomy_category qPrefGeocoding::firstTaxonomyCategory() const
{
	return prefs.geocoding.category[0];
}
void qPrefGeocoding::setFirstTaxonomyCategory(taxonomy_category value)
{
	if (value != prefs.geocoding.category[0]) {
		prefs.geocoding.category[0] = value;
		diskFirstTaxonomyCategory(true);
		emit firstTaxonomyCategoryChanged(value);
	}
}
void qPrefGeocoding::diskFirstTaxonomyCategory(bool doSync)
{
	LOADSYNC_ENUM("/cat0", taxonomy_category, geocoding.category[0]);
}


taxonomy_category qPrefGeocoding::secondTaxonomyCategory() const
{
	return prefs.geocoding.category[1];
}
void qPrefGeocoding::setSecondTaxonomyCategory(taxonomy_category value)
{
	if (value != prefs.geocoding.category[1]) {
		prefs.geocoding.category[1]= value;
		diskSecondTaxonomyCategory(true);
		emit secondTaxonomyCategoryChanged(value);
	}
}
void qPrefGeocoding::diskSecondTaxonomyCategory(bool doSync)
{
	LOADSYNC_ENUM("/cat1", taxonomy_category, geocoding.category[1]);
}


taxonomy_category qPrefGeocoding::thirdTaxonomyCategory() const
{
	return prefs.geocoding.category[2];
}
void qPrefGeocoding::setThirdTaxonomyCategory(taxonomy_category value)
{
	if (value != prefs.geocoding.category[2]) {
		prefs.geocoding.category[2] = value;
		diskThirdTaxonomyCategory(true);
		emit thirdTaxonomyCategoryChanged(value);
	}
}
void qPrefGeocoding::diskThirdTaxonomyCategory(bool doSync)
{
	LOADSYNC_ENUM("/cat2", taxonomy_category, geocoding.category[2]);
}
