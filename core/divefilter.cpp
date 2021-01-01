// SPDX-License-Identifier: GPL-2.0

#include "divefilter.h"
#include "divelist.h"
#include "gettextfromc.h"
#include "qthelper.h"
#include "selection.h"
#include "subsurface-qt/divelistnotifier.h"
#if !defined(SUBSURFACE_MOBILE) && !defined(SUBSURFACE_DOWNLOADER)
#include "desktop-widgets/mapwidget.h"
#include "desktop-widgets/mainwindow.h"
#include "desktop-widgets/divelistview.h"
#include "qt-models/filtermodels.h"
#endif

// Set filter status of dive and return whether it has been changed
bool DiveFilter::setFilterStatus(struct dive *d, bool shown) const
{
	bool old_shown, changed;
	if (!d)
		return false;
	old_shown = !d->hidden_by_filter;
	d->hidden_by_filter = !shown;
	if (!shown && d->selected)
		deselect_dive(d);
	changed = old_shown != shown;
	if (changed)
		shown_dives += shown - old_shown;
	return changed;
}

void DiveFilter::updateDiveStatus(dive *d, bool newStatus, ShownChange &change) const
{
	if (setFilterStatus(d, newStatus)) {
		if (newStatus)
			change.newShown.push_back(d);
		else
			change.newHidden.push_back(d);
	}
}

bool FilterData::operator==(const FilterData &f2) const
{
	return fullText.originalQuery == f2.fullText.originalQuery &&
	       fulltextStringMode == f2.fulltextStringMode &&
	       constraints == f2.constraints;
}

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

void DiveFilter::reset()
{
	int i;
	dive *d;
	shown_dives = dive_table.nr;
	for_each_dive(i, d)
		d->hidden_by_filter = false;
	updateAll();
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

DiveFilter::DiveFilter() :
	shown_dives(0),
	diveSiteRefCount(0)
{
}

void DiveFilter::diveRemoved(const dive *d) const
{
	if (!d->hidden_by_filter)
		--shown_dives;
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

#if !defined(SUBSURFACE_MOBILE) && !defined(SUBSURFACE_DOWNLOADER)
void DiveFilter::startFilterDiveSites(QVector<dive_site *> ds)
{
	if (++diveSiteRefCount > 1) {
		setFilterDiveSite(ds);
	} else {
		std::sort(ds.begin(), ds.end());
		dive_sites = ds;
		// When switching into dive site mode, reload the dive sites.
		// TODO: why here? why not catch the filterReset signal in the map widget
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
#else
bool DiveFilter::diveSiteMode() const
{
	return false;
}
#endif

QString DiveFilter::shownText() const
{
	if (diveSiteMode() || filterData.validFilter())
		return gettextFromC::tr("%L1/%L2 shown").arg(shown_dives).arg(dive_table.nr);
	else
		return gettextFromC::tr("%L1 dives").arg(dive_table.nr);
}

int DiveFilter::shownDives() const
{
	return shown_dives;
}

void DiveFilter::setFilter(const FilterData &data)
{
	filterData = data;
	emit diveListNotifier.filterReset();
}

std::vector<dive *> DiveFilter::visibleDives() const
{
	if (shown_dives <= 0)
		return {};

	std::vector<dive *> res;
	res.reserve(shown_dives);

	int i;
	dive *d;
	for_each_dive(i, d) {
		if (!d->hidden_by_filter)
			res.push_back(d);
	}
	return res;
}
