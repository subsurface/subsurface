// SPDX-License-Identifier: GPL-2.0
// A class that filters dives.
#ifndef DIVE_FILTER_H
#define DIVE_FILTER_H

#include <QVector>
struct dive;

// Structure describing changes of shown status upon applying the filter
struct ShownChange {
	QVector<dive *> newShown;
	QVector<dive *> newHidden;
	bool currentChanged;
};

// The dive filter for mobile is currently much simpler than for desktop.
// Therefore, for now we have two completely separate implementations.
// This should be unified in the future.
#ifdef SUBSURFACE_MOBILE

class DiveFilter {
public:
	static DiveFilter *instance();

	ShownChange update(const QVector<dive *> &dives) const; // Update filter status of given dives and return dives whose status changed
	ShownChange updateAll() const; // Update filter status of all dives and return dives whose status changed
private:
	DiveFilter();
};

#else

#include "fulltext.h"
#include <QDateTime>
#include <QStringList>

struct dive_trip;
struct dive_site;

struct FilterData {
	// The mode ids are chosen such that they can be directly converted from / to combobox indices.
	enum class Mode {
		ALL_OF = 0,
		ANY_OF = 1,
		NONE_OF = 2
	};

	bool validFilter = false;
	int minVisibility = 0;
	int maxVisibility = 5;
	int minRating = 0;
	int maxRating = 5;
	// The default minimum and maximum temperatures are set such that all
	// physically reasonable dives are shown. Note that these values should
	// work for both Celsius and Fahrenheit scales.
	double minWaterTemp = -10;
	double maxWaterTemp = 200;
	double minAirTemp = -50;
	double maxAirTemp = 200;
	QDateTime fromDate = QDateTime(QDate(1980,1,1));
	QTime fromTime = QTime(0,0);
	QDateTime toDate = QDateTime::currentDateTime();
	QTime toTime = QTime::currentTime();
	QStringList tags;
	QStringList people;
	QStringList location;
	QStringList suit;
	QStringList dnotes;
	QStringList equipment;
	FullTextQuery fullText;
	Mode tagsMode = Mode::ALL_OF;
	Mode peopleMode = Mode::ALL_OF;
	Mode locationMode = Mode::ANY_OF;
	Mode dnotesMode = Mode::ALL_OF;
	Mode suitMode = Mode::ANY_OF;
	Mode equipmentMode = Mode::ALL_OF;
	StringFilterMode fulltextStringMode = StringFilterMode::STARTSWITH;
	StringFilterMode tagsStringMode = StringFilterMode::SUBSTRING;
	StringFilterMode peopleStringMode = StringFilterMode::SUBSTRING;
	StringFilterMode locationStringMode = StringFilterMode::SUBSTRING;
	StringFilterMode dnotesStringMode = StringFilterMode::SUBSTRING;
	StringFilterMode suitStringMode = StringFilterMode::SUBSTRING;
	StringFilterMode equipmentStringMode = StringFilterMode::SUBSTRING;
	bool logged = true;
	bool planned = true;
};

class DiveFilter {
public:
	static DiveFilter *instance();

	bool diveSiteMode() const; // returns true if we're filtering on dive site
	const QVector<dive_site *> &filteredDiveSites() const;
	void startFilterDiveSites(QVector<dive_site *> ds);
	void setFilterDiveSite(QVector<dive_site *> ds);
	void stopFilterDiveSites();
	void setFilter(const FilterData &data);
	ShownChange update(const QVector<dive *> &dives) const; // Update filter status of given dives and return dives whose status changed
	ShownChange updateAll() const; // Update filter status of all dives and return dives whose status changed
private:
	DiveFilter();
	bool showDive(const struct dive *d) const; // Should that dive be shown?

	QVector<dive_site *> dive_sites;
	FilterData filterData;

	// We use ref-counting for the dive site mode. The reason is that when switching
	// between two tabs that both need dive site mode, the following course of
	// events may happen:
	//	1) The new tab appears -> enter dive site mode.
	//	2) The old tab gets its hide() signal -> exit dive site mode.
	// The filter is now not in dive site mode, even if it should
	int diveSiteRefCount;
};
#endif // SUBSURFACE_MOBILE

#endif
