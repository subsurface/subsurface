// SPDX-License-Identifier: GPL-2.0

#include "divefilter.h"
#include "divelist.h" // for filter_dive
#include "qthelper.h"
#include "subsurface-qt/divelistnotifier.h"

static void updateDiveStatus(dive *d, bool newStatus, ShownChange &change)
{
	if (filter_dive(d, newStatus)) {
		if (newStatus)
			change.newShown.push_back(d);
		else
			change.newHidden.push_back(d);
	}
}

#ifdef SUBSURFACE_MOBILE

#include "tag.h"
static QStringList getTagList(const dive *d)
{
	QStringList res;
	for (const tag_entry *tag = d->tag_list; tag; tag = tag->next)
		res.push_back(QString(tag->tag->name).trimmed());
	res.append(gettextFromC::tr(divemode_text_ui[d->dc.divemode]));
	return res;
}

// Check if a string-list contains at least one string that starts with the second argument.
// Comparison is non case sensitive and removes white space.
static bool listContainsSuperstring(const QStringList &list, const QString &s)
{
	return std::any_of(list.begin(), list.end(), [&s](const QString &s2)
			   { return s2.startsWith(s, Qt::CaseInsensitive); } );
}

static bool check(const QStringList &items, const QStringList &list)
{
	return std::all_of(items.begin(), items.end(), [&list](const QString &item)
			   { return listContainsSuperstring(list, item); });
}

static bool hasTags(const QStringList &tags, const struct dive *d)
{
	if (tags.isEmpty())
		return true;
	return check(tags, getTagList(d));
}

static bool hasPersons(const QStringList &people, const struct dive *d)
{
	if (people.isEmpty())
		return true;
	QStringList dive_people = QString(d->buddy).split(",", QString::SkipEmptyParts)
		+ QString(d->divemaster).split(",", QString::SkipEmptyParts);
	return check(people, dive_people);
}

DiveFilter *DiveFilter::instance()
{
	static DiveFilter self;
	return &self;
}

DiveFilter::DiveFilter()
{
}

ShownChange DiveFilter::update(const QVector<dive *> &dives) const
{
	dive *old_current = current_dive;

	ShownChange res;
	switch (filterData.mode) {
	default:
	case FilterData::Mode::NONE:
		for (dive *d: dives)
			updateDiveStatus(d, true, res);
		break;
	case FilterData::Mode::FULLTEXT:
		for (dive *d: dives)
			updateDiveStatus(d, fulltext_dive_matches(d, filterData.fullText, StringFilterMode::STARTSWITH), res);
		break;
	case FilterData::Mode::PEOPLE:
		for (dive *d: dives)
			updateDiveStatus(d, hasPersons(filterData.tags, d), res);
		break;
	case FilterData::Mode::TAGS:
		for (dive *d: dives)
			updateDiveStatus(d, hasTags(filterData.tags, d), res);
		break;
	}

	res.currentChanged = old_current != current_dive;
	return res;
}

ShownChange DiveFilter::updateAll() const
{
	dive *old_current = current_dive;

	ShownChange res;
	int i;
	dive *d;
	switch (filterData.mode) {
	default:
	case FilterData::Mode::NONE:
		for_each_dive(i, d)
			updateDiveStatus(d, true, res);
		break;
	case FilterData::Mode::FULLTEXT: {
		FullTextResult ft = fulltext_find_dives(filterData.fullText, StringFilterMode::STARTSWITH);
		for_each_dive(i, d)
			updateDiveStatus(d, ft.dive_matches(d), res);
		break;
	}
	case FilterData::Mode::PEOPLE:
		for_each_dive(i, d)
			updateDiveStatus(d, hasPersons(filterData.tags, d), res);
		break;
	case FilterData::Mode::TAGS:
		for_each_dive(i, d)
			updateDiveStatus(d, hasTags(filterData.tags, d), res);
		break;
	}

	res.currentChanged = old_current != current_dive;
	return res;
}

void DiveFilter::setFilter(const FilterData &data)
{
	filterData = data;
	emit diveListNotifier.filterReset();
}

#else // SUBSURFACE_MOBILE

#include "desktop-widgets/mapwidget.h"
#include "desktop-widgets/mainwindow.h"
#include "desktop-widgets/divelistview.h"
#include "qt-models/filtermodels.h"

bool FilterData::validFilter() const
{
	return fullText.doit() || !constraints.empty();
}

ShownChange DiveFilter::update(const QVector<dive *> &dives) const
{
	dive *old_current = current_dive;

	ShownChange res;
	bool doDS = diveSiteMode();
	bool doFullText = filterData.fullText.doit();
	for (dive *d: dives) {
		// There are three modes: divesite, fulltext, normal
		bool newStatus = doDS        ? dive_sites.contains(d->dive_site) :
				 doFullText  ? fulltext_dive_matches(d, filterData.fullText, filterData.fulltextStringMode) && showDive(d) :
					       showDive(d);
		updateDiveStatus(d, newStatus, res);
	}
	res.currentChanged = old_current != current_dive;
	return res;
}

ShownChange DiveFilter::updateAll() const
{
	dive *old_current = current_dive;

	ShownChange res;
	int i;
	dive *d;
	// There are three modes: divesite, fulltext, normal
	if (diveSiteMode()) {
		for_each_dive(i, d) {
			bool newStatus = dive_sites.contains(d->dive_site);
			updateDiveStatus(d, newStatus, res);
		}
	} else if (filterData.fullText.doit()) {
		FullTextResult ft = fulltext_find_dives(filterData.fullText, filterData.fulltextStringMode);
		for_each_dive(i, d) {
			bool newStatus = ft.dive_matches(d) && showDive(d);
			updateDiveStatus(d, newStatus, res);
		}
	} else {
		for_each_dive(i, d) {
			bool newStatus = showDive(d);
			updateDiveStatus(d, newStatus, res);
		}
	}
	res.currentChanged = old_current != current_dive;
	return res;
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
	if (d->invalid && !prefs.display_invalid_dives)
		return false;

	if (!filterData.validFilter())
		return true;

	return std::all_of(filterData.constraints.begin(), filterData.constraints.end(),
			   [d] (const filter_constraint &c) { return filter_constraint_match_dive(c, d); });
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
