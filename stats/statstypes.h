// SPDX-License-Identifier: GPL-2.0
// Types used for the statistic widgets. There are three kinds of types:
// 1) Discrete types can only adopt discrete values.
//    Examples are dive-type or dive buddy.
//    Note that for example dive buddy means that a dive can have
//    multiple values.
// 2) Continuous types have a notion of a linear distance and can be
//    plotted on a linear axis.
//    An Example is the dive-date.
// 3) Numeric types are continuous types that support operations
//    such as averaging.
// Not every type makes sense in every type of graph.
#ifndef STATS_TYPES_H
#define STATS_TYPES_H

#include <vector>
#include <memory>
#include <QString>
#include <QObject>

struct dive;

// Operations that can be performed on numeric types
enum class StatsOperation : int {
	Median = 0,
	Average,
	TimeWeightedAverage,
	Sum
};

// For median and quartiles.
struct StatsQuartiles {
	double min;
	double q1, q2, q3;
	double max;
};

struct StatsBin {
	virtual ~StatsBin();
	virtual bool operator<(StatsBin &) const = 0;
	virtual bool operator==(StatsBin &) const = 0;
	bool operator!=(StatsBin &b) const { return !(*this == b); }
};

using StatsBinPtr = std::unique_ptr<StatsBin>;

struct StatsBinDives {
	StatsBinPtr bin;
	std::vector<dive *> dives;
};

struct StatsBinCount {
	StatsBinPtr bin;
	int count;
};

struct StatsBinner {
	virtual ~StatsBinner();
	virtual QString name() const; // Only needed if there are multiple binners for a type
	virtual QString unitSymbol() const; // For numeric types - by default returns empty string

	// The binning functions have a parameter "fill_empty". If true, missing
	// bins in the range will be filled with empty bins. This only works for continuous types.
	virtual std::vector<StatsBinDives> bin_dives(const std::vector<dive *> &dives, bool fill_empty) const = 0;
	virtual std::vector<StatsBinCount> count_dives(const std::vector<dive *> &dives, bool fill_empty) const = 0;

	// Note: these functions will crash with an exception if passed incompatible bins!
	virtual QString format(const StatsBin &bin) const = 0;
	virtual QString formatLowerBound(const StatsBin &bin) const; // Only for continuous types
	virtual QString formatUpperBound(const StatsBin &bin) const; // Only for continuous types
	virtual double lowerBoundToFloat(const StatsBin &bin) const; // Only for continuous types
	virtual double upperBoundToFloat(const StatsBin &bin) const; // Only for continuous types

	// Only for continuous and numeric types
	// Note: this will crash with an exception if passed incompatible bins!
	virtual std::vector<StatsBinPtr> bins_between(const StatsBin &bin1, const StatsBin &bin2) const;
};

struct StatsType {
	enum class Type {
		Discrete,
		Continuous,
		Numeric
	};

	virtual ~StatsType();
	virtual Type type() const = 0;
	virtual QString name() const = 0;
	virtual QString unitSymbol() const; // For numeric types - by default returns empty string
	virtual int decimals() const; // For numeric types: numbers of decimals to display on axes. Defaults to 0.
	virtual std::vector<const StatsBinner *> binners() const = 0; // Note: may depend on current locale!
	const StatsBinner *getBinner(int idx) const; // Handles out of bounds gracefully (returns first binner)
	QString nameWithUnit() const;
	QString nameWithBinnerUnit(const StatsBinner &) const;
	virtual std::vector<StatsOperation> supportedOperations() const; // Only for numeric types
	QStringList supportedOperationNames() const; // Only for numeric types
	StatsOperation idxToOperation(int idx) const;
	static QString operationName(StatsOperation);
	double average(const std::vector<dive *> &dives) const;
	double averageTimeWeighted(const std::vector<dive *> &dives) const;
	static StatsQuartiles quartiles(const std::vector<double> &values);
	StatsQuartiles quartiles(const std::vector<dive *> &dives) const;
	std::vector<double> values(const std::vector<dive *> &dives) const;
	std::vector<std::pair<double,double>> scatter(const StatsType &t2, const std::vector<dive *> &dives) const;
	double sum(const std::vector<dive *> &dives) const;
	double applyOperation(const std::vector<dive *> &dives, StatsOperation op) const;
private:
	virtual double toFloat(const struct dive *d) const; // For numeric types - if dive doesn't have that value, returns NaN
};

extern const std::vector<const StatsType *> stats_types;
extern const std::vector<const StatsType *> stats_continuous_types; // includes numeric types
extern const std::vector<const StatsType *> stats_numeric_types; // types that support averaging, etc

// Dummy object for our translations
class StatsTranslations : public QObject
{
	Q_OBJECT
};

#endif
