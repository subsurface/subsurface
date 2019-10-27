// SPDX-License-Identifier: GPL-2.0
#include "qt-models/filtermodels.h"
#include "qt-models/models.h"
#include "core/display.h"
#include "core/qthelper.h"
#include "core/divesite.h"
#include "core/trip.h"
#include "core/subsurface-string.h"
#include "core/subsurface-qt/DiveListNotifier.h"
#include "qt-models/divetripmodel.h"

#if !defined(SUBSURFACE_MOBILE)
#include "desktop-widgets/divelistview.h"
#include "desktop-widgets/mainwindow.h"
#include "desktop-widgets/mapwidget.h"
#endif

#include <QDebug>
#include <algorithm>

namespace {
	// Check if a string-list contains at least one string containing the second argument.
	// Comparison is non case sensitive and removes white space.
	bool listContainsSuperstring(const QStringList &list, const QString &s)
	{
		return std::any_of(list.begin(), list.end(), [&s](const QString &s2)
				   { return s2.trimmed().contains(s.trimmed(), Qt::CaseInsensitive); } );
	}

	// Check whether either all, any or none of the items of the first list is
	// in the second list as a super string.
	// The mode is controlled by the second argument
	bool check(const QStringList &items, const QStringList &list, FilterData::Mode mode)
	{
		bool negate = mode == FilterData::Mode::NONE_OF;
		bool any_of = mode == FilterData::Mode::ANY_OF;
		auto fun = [&list, negate](const QString &item)
			   { return listContainsSuperstring(list, item) != negate; };
		return any_of ? std::any_of(items.begin(), items.end(), fun)
			      : std::all_of(items.begin(), items.end(), fun);
	}

	bool hasTags(const QStringList &tags, const struct dive *d, FilterData::Mode mode)
	{
		if (tags.isEmpty())
			return true;
		QStringList dive_tags = get_taglist_string(d->tag_list).split(",");
		dive_tags.append(gettextFromC::tr(divemode_text_ui[d->dc.divemode]));
		return check(tags, dive_tags, mode);
	}

	bool hasPersons(const QStringList &people, const struct dive *d, FilterData::Mode mode)
	{
		if (people.isEmpty())
			return true;
		QStringList dive_people = QString(d->buddy).split(",", QString::SkipEmptyParts)
			+ QString(d->divemaster).split(",", QString::SkipEmptyParts);
		return check(people, dive_people, mode);
	}

	bool hasLocations(const QStringList &locations, const struct dive *d, FilterData::Mode mode)
	{
		if (locations.isEmpty())
			return true;
		QStringList diveLocations;
		if (d->divetrip)
			diveLocations.push_back(QString(d->divetrip->location));

		if (d->dive_site)
			diveLocations.push_back(QString(d->dive_site->name));

		return check(locations, diveLocations, mode);
	}

	// TODO: Finish this implementation.
	bool hasEquipment(const QStringList &, const struct dive *, FilterData::Mode)
	{
		return true;
	}

	bool hasSuits(const QStringList &suits, const struct dive *d, FilterData::Mode mode)
	{
		if (suits.isEmpty())
			return true;
		QStringList diveSuits;
		if (d->suit)
			diveSuits.push_back(QString(d->suit));
		return check(suits, diveSuits, mode);
	}

	bool hasNotes(const QStringList &dnotes, const struct dive *d, FilterData::Mode mode)
	{
		if (dnotes.isEmpty())
			return true;
		QStringList diveNotes;
		if (d->notes)
			diveNotes.push_back(QString(d->notes));
		return check(dnotes, diveNotes, mode);
	}

}

MultiFilterSortModel *MultiFilterSortModel::instance()
{
	static MultiFilterSortModel self;
	return &self;
}

MultiFilterSortModel::MultiFilterSortModel(QObject *parent) : QSortFilterProxyModel(parent),
	divesDisplayed(0),
	diveSiteRefCount(0)
{
	setFilterKeyColumn(-1); // filter all columns
	setFilterCaseSensitivity(Qt::CaseInsensitive);

	connect(&diveListNotifier, &DiveListNotifier::divesAdded, this, &MultiFilterSortModel::divesAdded);
	connect(&diveListNotifier, &DiveListNotifier::divesDeleted, this, &MultiFilterSortModel::divesDeleted);
}

void MultiFilterSortModel::resetModel(DiveTripModelBase::Layout layout)
{
	DiveTripModelBase::resetModel(layout);
	// DiveTripModelBase::resetModel() generates a new instance.
	// Thus, the source model must be reset.
	setSourceModel(DiveTripModelBase::instance());
}

bool MultiFilterSortModel::showDive(const struct dive *d) const
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
	if (!hasTags(filterData.tags, d, filterData.tagsMode))
		return false;

	// people
	if (!hasPersons(filterData.people, d, filterData.peopleMode))
		return false;

	// Location
	if (!hasLocations(filterData.location, d, filterData.locationMode))
		return false;

	// Suit
	if (!hasSuits(filterData.suit, d, filterData.suitMode))
		return false;

	// Notes
	if (!hasNotes(filterData.dnotes, d, filterData.dnotesMode))
		return false;

	if (!hasEquipment(filterData.equipment, d, filterData.equipmentMode))
		return false;

	// Planned/Logged
	if (!filterData.logged && !has_planned(d, true))
		return false;
	if (!filterData.planned && !has_planned(d, false))
		return false;

	return true;
}

bool MultiFilterSortModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
	QAbstractItemModel *m = sourceModel();
	QModelIndex index0 = m->index(source_row, 0, source_parent);
	return m->data(index0, DiveTripModelBase::SHOWN_ROLE).value<bool>();
}

void MultiFilterSortModel::filterChanged(const QModelIndex &from, const QModelIndex &to, const QVector<int> &roles)
{
	// Only redo the filter if a checkbox changed. If the count of an entry changed,
	// we do *not* want to recalculate the filters.
	if (roles.contains(Qt::CheckStateRole))
		myInvalidate();
}

void MultiFilterSortModel::myInvalidate()
{
	QAbstractItemModel *m = sourceModel();
	if (!m)
		return;

	{
		// This marker prevents the UI from getting notifications on selection changes.
		// It is active until the end of the scope.
		// This is actually meant for the undo-commands, so that they can do their work
		// without having the UI updated.
		// Here, it is used because invalidating the filter can generate numerous
		// selection changes, which do full ui reloads. Instead, do that all at once
		// as a consequence of the filterFinished signal right after the local scope.
		auto marker = diveListNotifier.enterCommand();

		divesDisplayed = 0;

		for (int i = 0; i < m->rowCount(QModelIndex()); ++i) {
			QModelIndex idx = m->index(i, 0, QModelIndex());

			dive_trip *trip = m->data(idx, DiveTripModelBase::TRIP_ROLE).value<dive_trip *>();
			if (trip) {
				// This is a trip -> loop over all dives and see if any is selected

				bool showTrip = false;
				for (int j = 0; j < m->rowCount(idx); ++j) {
					QModelIndex idx2 = m->index(j, 0, idx);
					dive *d = m->data(idx2, DiveTripModelBase::DIVE_ROLE).value<dive *>();
					if (!d) {
						qWarning("MultiFilterSortModel::myInvalidate(): subitem not a dive!?");
						continue;
					}
					bool show = showDive(d);
					if (show) {
						divesDisplayed++;
						showTrip = true;
					}
					m->setData(idx2, show, DiveTripModelBase::SHOWN_ROLE);
				}
				m->setData(idx, showTrip, DiveTripModelBase::SHOWN_ROLE);
			} else {
				dive *d = m->data(idx, DiveTripModelBase::DIVE_ROLE).value<dive *>();
				bool show = (d != NULL) && showDive(d);
				if (show)
					divesDisplayed++;
				m->setData(idx, show, DiveTripModelBase::SHOWN_ROLE);
			}
		}

		invalidateFilter();

		// Tell the dive trip model to update the displayed-counts
		DiveTripModelBase::instance()->filterFinished();
		countsChanged();
	}

#if !defined(SUBSURFACE_MOBILE)
	// The shown maps may have changed -> reload the map widget.
	// But don't do this in dive site mode, because then we show all
	// dive sites and only change the selected flag.
	if (!diveSiteMode())
		MapWidget::instance()->reload();
#endif

	emit filterFinished();

#if !defined(SUBSURFACE_MOBILE)
	if (diveSiteMode())
		MainWindow::instance()->diveList->expandAll();
#endif
}

bool MultiFilterSortModel::updateDive(struct dive *d)
{
	bool oldStatus = !d->hidden_by_filter;
	bool newStatus = showDive(d);
	bool changed = oldStatus != newStatus;
	if (changed) {
		filter_dive(d, newStatus);
		divesDisplayed += newStatus - oldStatus;
	}
	return changed;
}

void MultiFilterSortModel::clearFilter()
{
	myInvalidate();
}

void MultiFilterSortModel::startFilterDiveSites(QVector<dive_site *> ds)
{
	if (++diveSiteRefCount > 1) {
		setFilterDiveSite(ds);
	} else {
		std::sort(ds.begin(), ds.end());
		dive_sites = ds;
#if !defined(SUBSURFACE_MOBILE)
		// When switching into dive site mode, reload the dive sites.
		// We won't do this in myInvalidate() once we are in dive site mode.
		MapWidget::instance()->reload();
#endif
		myInvalidate();
	}
}

void MultiFilterSortModel::stopFilterDiveSites()
{
	if (--diveSiteRefCount > 0)
		return;
	dive_sites.clear();
	myInvalidate();
}

void MultiFilterSortModel::setFilterDiveSite(QVector<dive_site *> ds)
{
	// If the filter didn't change, return early to avoid a full
	// map reload. For a well-defined comparison, sort the vector first.
	std::sort(ds.begin(), ds.end());
	if (ds == dive_sites)
		return;
	dive_sites = ds;

#if !defined(SUBSURFACE_MOBILE)
	MapWidget::instance()->setSelected(dive_sites);
#endif
	myInvalidate();
}

const QVector<dive_site *> &MultiFilterSortModel::filteredDiveSites() const
{
	return dive_sites;
}

bool MultiFilterSortModel::diveSiteMode() const
{
	return diveSiteRefCount > 0;
}

bool MultiFilterSortModel::lessThan(const QModelIndex &i1, const QModelIndex &i2) const
{
	// Hand sorting down to the source model.
	return DiveTripModelBase::instance()->lessThan(i1, i2);
}

void MultiFilterSortModel::filterDataChanged(const FilterData &data)
{
	filterData = data;
	myInvalidate();
}

void MultiFilterSortModel::divesAdded(dive_trip *, bool, const QVector<dive *> &dives)
{
	for (dive *d: dives) {
		if (!d->hidden_by_filter)
			++divesDisplayed;
	}
	countsChanged();
}

void MultiFilterSortModel::divesDeleted(dive_trip *, bool, const QVector<dive *> &dives)
{
	for (dive *d: dives) {
		if (!d->hidden_by_filter)
			--divesDisplayed;
	}
	countsChanged();
}

void MultiFilterSortModel::countsChanged()
{
	updateWindowTitle();
}
