// SPDX-License-Identifier: GPL-2.0
// A class that filters dives.
#ifndef DIVE_FILTER_H
#define DIVE_FILTER_H

#include "fulltext.h"
#include "filterconstraint.h"
#include <vector>
#include <QVector>
#include <QStringList>

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

struct FilterData {
	// On mobile, we support searching fulltext (all fields), people (buddies and divemasters) and tags
	enum class Mode {
		NONE = 0,
		FULLTEXT = 1,
		PEOPLE = 2,
		TAGS = 3
	};

	Mode mode = Mode::NONE;
	FullTextQuery fullText; // For fulltext
	QStringList tags; // For people and tags
};

class DiveFilter {
public:
	static DiveFilter *instance();

	ShownChange update(const QVector<dive *> &dives) const; // Update filter status of given dives and return dives whose status changed
	ShownChange updateAll() const; // Update filter status of all dives and return dives whose status changed
	void setFilter(const FilterData &data);
private:
	DiveFilter();

	FilterData filterData;
};

#else

struct dive_trip;
struct dive_site;

struct FilterData {
	// The mode ids are chosen such that they can be directly converted from / to combobox indices.
	enum class Mode {
		ALL_OF = 0,
		ANY_OF = 1,
		NONE_OF = 2
	};

	FullTextQuery fullText;
	StringFilterMode fulltextStringMode = StringFilterMode::STARTSWITH;
	std::vector<filter_constraint> constraints;
	bool validFilter() const;
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
