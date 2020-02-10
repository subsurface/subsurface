// SPDX-License-Identifier: GPL-2.0

#include "divefilter.h"

#ifdef SUBSURFACE_MOBILE

DiveFilter::DiveFilter()
{
}

bool DiveFilter::showDive(const struct dive *d) const
{
	// TODO: Do something useful
	return true;
}

#else // SUBSURFACE_MOBILE

#include "desktop-widgets/mapwidget.h"
#include "desktop-widgets/mainwindow.h"
#include "desktop-widgets/divelistview.h"
#include "core/qthelper.h"
#include "core/trip.h"
#include "core/divesite.h"
#include "qt-models/filtermodels.h"

namespace {
	// Pointer to function that takes two strings and returns whether
	// the first matches the second according to a criterion (substring, starts-with, exact).
	using StrCheck = bool (*) (const QString &s1, const QString &s2);

	// Check if a string-list contains at least one string containing the second argument.
	// Comparison is non case sensitive and removes white space.
	bool listContainsSuperstring(const QStringList &list, const QString &s, StrCheck strchk)
	{
		return std::any_of(list.begin(), list.end(), [&s,strchk](const QString &s2)
				   { return strchk(s2, s); } );
	}

	// Check whether either all, any or none of the items of the first list is
	// in the second list as a super string.
	// The mode is controlled by the second argument
	bool check(const QStringList &items, const QStringList &list, FilterData::Mode mode, FilterData::StringMode stringMode)
	{
		bool negate = mode == FilterData::Mode::NONE_OF;
		bool any_of = mode == FilterData::Mode::ANY_OF;
		StrCheck strchk =
			stringMode == FilterData::StringMode::SUBSTRING ?
				[](const QString &s1, const QString &s2) { return s1.contains(s2, Qt::CaseInsensitive); } :
			stringMode == FilterData::StringMode::STARTSWITH ?
				[](const QString &s1, const QString &s2) { return s1.startsWith(s2, Qt::CaseInsensitive); } :
			/* FilterData::StringMode::EXACT */
				[](const QString &s1, const QString &s2) { return s1.compare(s2, Qt::CaseInsensitive) == 0; };
		auto fun = [&list, negate, strchk](const QString &item)
			   { return listContainsSuperstring(list, item, strchk) != negate; };
		return any_of ? std::any_of(items.begin(), items.end(), fun)
			      : std::all_of(items.begin(), items.end(), fun);
	}

	bool hasTags(const QStringList &tags, const struct dive *d, FilterData::Mode mode, FilterData::StringMode stringMode)
	{
		if (tags.isEmpty())
			return true;
		QStringList dive_tags = get_taglist_string(d->tag_list).split(",");
		dive_tags.append(gettextFromC::tr(divemode_text_ui[d->dc.divemode]));
		return check(tags, dive_tags, mode, stringMode);
	}

	bool hasPersons(const QStringList &people, const struct dive *d, FilterData::Mode mode, FilterData::StringMode stringMode)
	{
		if (people.isEmpty())
			return true;
		QStringList dive_people = QString(d->buddy).split(",", QString::SkipEmptyParts)
			+ QString(d->divemaster).split(",", QString::SkipEmptyParts);
		return check(people, dive_people, mode, stringMode);
	}

	bool hasLocations(const QStringList &locations, const struct dive *d, FilterData::Mode mode, FilterData::StringMode stringMode)
	{
		if (locations.isEmpty())
			return true;
		QStringList diveLocations;
		if (d->divetrip)
			diveLocations.push_back(QString(d->divetrip->location));

		if (d->dive_site)
			diveLocations.push_back(QString(d->dive_site->name));

		return check(locations, diveLocations, mode, stringMode);
	}

	// TODO: Finish this implementation.
	bool hasEquipment(const QStringList &, const struct dive *, FilterData::Mode, FilterData::StringMode)
	{
		return true;
	}

	bool hasSuits(const QStringList &suits, const struct dive *d, FilterData::Mode mode, FilterData::StringMode stringMode)
	{
		if (suits.isEmpty())
			return true;
		QStringList diveSuits;
		if (d->suit)
			diveSuits.push_back(QString(d->suit));
		return check(suits, diveSuits, mode, stringMode);
	}

	bool hasNotes(const QStringList &dnotes, const struct dive *d, FilterData::Mode mode, FilterData::StringMode stringMode)
	{
		if (dnotes.isEmpty())
			return true;
		QStringList diveNotes;
		if (d->notes)
			diveNotes.push_back(QString(d->notes));
		return check(dnotes, diveNotes, mode, stringMode);
	}
}

DiveFilter *DiveFilter::instance()
{
	static DiveFilter self;
	return &self;
}

DiveFilter::DiveFilter() : diveSiteRefCount(0)
{
}

bool DiveFilter::showDive(const struct dive *d) const
{
	if (diveSiteMode())
		return dive_sites.contains(d->dive_site);

	if (!filterData.validFilter)
		return true;

	if (d->visibility < filterData.minVisibility || d->visibility > filterData.maxVisibility)
		return false;

	if (d->rating < filterData.minRating || d->rating > filterData.maxRating)
		return false;

	auto temp_comp = prefs.units.temperature == units::CELSIUS ? C_to_mkelvin : F_to_mkelvin;
	if (d->watertemp.mkelvin &&
	    (d->watertemp.mkelvin < (*temp_comp)(filterData.minWaterTemp) || d->watertemp.mkelvin > (*temp_comp)(filterData.maxWaterTemp)))
		return false;

	if (d->airtemp.mkelvin &&
	    (d->airtemp.mkelvin < (*temp_comp)(filterData.minAirTemp) || d->airtemp.mkelvin > (*temp_comp)(filterData.maxAirTemp)))
		return false;

	QDateTime t = filterData.fromDate;
	t.setTime(filterData.fromTime);
	if (filterData.fromDate.isValid() && filterData.fromTime.isValid() &&
	    d->when < t.toMSecsSinceEpoch()/1000 + t.offsetFromUtc())
		return false;

	t = filterData.toDate;
	t.setTime(filterData.toTime);
	if (filterData.toDate.isValid() && filterData.toTime.isValid() &&
	    d->when > t.toMSecsSinceEpoch()/1000 + t.offsetFromUtc())
		return false;

	// tags.
	if (!hasTags(filterData.tags, d, filterData.tagsMode, filterData.tagsStringMode))
		return false;

	// people
	if (!hasPersons(filterData.people, d, filterData.peopleMode, filterData.peopleStringMode))
		return false;

	// Location
	if (!hasLocations(filterData.location, d, filterData.locationMode, filterData.locationStringMode))
		return false;

	// Suit
	if (!hasSuits(filterData.suit, d, filterData.suitMode, filterData.suitStringMode))
		return false;

	// Notes
	if (!hasNotes(filterData.dnotes, d, filterData.dnotesMode, filterData.dnotesStringMode))
		return false;

	if (!hasEquipment(filterData.equipment, d, filterData.equipmentMode, filterData.equipmentStringMode))
		return false;

	// Planned/Logged
	if (!filterData.logged && !has_planned(d, true))
		return false;
	if (!filterData.planned && !has_planned(d, false))
		return false;

	return true;
}

void DiveFilter::startFilterDiveSites(QVector<dive_site *> ds)
{
	if (++diveSiteRefCount > 1) {
		setFilterDiveSite(ds);
	} else {
		std::sort(ds.begin(), ds.end());
		dive_sites = ds;
		// When switching into dive site mode, reload the dive sites.
		// We won't do this in myInvalidate() once we are in dive site mode.
		MapWidget::instance()->reload();
		emit diveListNotifier.filterReset();
	}
}

void DiveFilter::stopFilterDiveSites()
{
	if (--diveSiteRefCount > 0)
		return;
	dive_sites.clear();
	emit diveListNotifier.filterReset();
	MapWidget::instance()->reload();
}

void DiveFilter::setFilterDiveSite(QVector<dive_site *> ds)
{
	// If the filter didn't change, return early to avoid a full
	// map reload. For a well-defined comparison, sort the vector first.
	std::sort(ds.begin(), ds.end());
	if (ds == dive_sites)
		return;
	dive_sites = ds;

	emit diveListNotifier.filterReset();
	MapWidget::instance()->setSelected(dive_sites);
	MapWidget::instance()->selectionChanged();
	MainWindow::instance()->diveList->expandAll();
}

const QVector<dive_site *> &DiveFilter::filteredDiveSites() const
{
	return dive_sites;
}

bool DiveFilter::diveSiteMode() const
{
	return diveSiteRefCount > 0;
}

void DiveFilter::setFilter(const FilterData &data)
{
	filterData = data;
	emit diveListNotifier.filterReset();
}
#endif // SUBSURFACE_MOBILE
