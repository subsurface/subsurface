// SPDX-License-Identifier: GPL-2.0
// Variables displayed by the statistic widgets. There are three
// kinds of variables:
// 1) Discrete variables can only adopt discrete values.
//    Examples are dive-type or dive buddy.
//    Note that for example dive buddy means that a dive can have
//    multiple values.
// 2) Continuous variables have a notion of a linear distance and can be
//    plotted on a linear axis.
//    An Example is the dive-date.
// 3) Numeric variables are continuous variables that support operations
//    such as averaging.
#ifndef STATS_TYPES_H
#define STATS_TYPES_H

#include <vector>
#include <memory>
#include <QString>
#include <QObject>

struct dive;

// Operations that can be performed on numeric variables
enum class StatsOperation : int {
	Median = 0,
	Mean,
	TimeWeightedMean,
	Sum,
	Min,
	Max,
	Invalid
};

// Results of the above operations
struct StatsOperationResults {
	std::vector<dive *> dives;
	double median;
	double mean;
	double timeWeightedMean;
	double sum;
	double min;
	double max;
	StatsOperationResults(); // Initialize to invalid (e.g. no dives)
	bool isValid() const;
	double get(StatsOperation op) const;
};

// For median and quartiles.
struct StatsQuartiles {
	std::vector<dive *> dives;
	double min;
	double q1, q2, q3;
	double max;
	bool isValid() const;
};

struct StatsBin {
	virtual ~StatsBin();
	virtual bool operator<(StatsBin &) const = 0;
	virtual bool operator==(StatsBin &) const = 0;
	bool operator!=(StatsBin &b) const { return !(*this == b); }
};

using StatsBinPtr = std::unique_ptr<StatsBin>;

// A value and a dive
struct StatsValue {
	double v;
	dive *d;
};

// A bin and an arbitrarily associated value, e.g. a count or a list of dives.
template<typename T>
struct StatsBinValue {
	StatsBinPtr bin;
	T value;
};
using StatsBinDives = StatsBinValue<std::vector<dive *>>;
using StatsBinValues = StatsBinValue<std::vector<StatsValue>>;
using StatsBinQuartiles = StatsBinValue<StatsQuartiles>;
using StatsBinOp = StatsBinValue<StatsOperationResults>;

struct StatsBinner {
	virtual ~StatsBinner();
	virtual QString name() const; // Only needed if there are multiple binners for a variable
	virtual QString unitSymbol() const; // For numeric variables - by default returns empty string

	// The binning functions have a parameter "fill_empty". If true, missing
	// bins in the range will be filled with empty bins. This only works for continuous variables.
	virtual std::vector<StatsBinDives> bin_dives(const std::vector<dive *> &dives, bool fill_empty) const = 0;

	// Note: these functions will crash with an exception if passed incompatible bins!
	virtual QString format(const StatsBin &bin) const = 0;
	QString formatWithUnit(const StatsBin &bin) const;
	virtual QString formatLowerBound(const StatsBin &bin) const; // Only for continuous variables
	virtual QString formatUpperBound(const StatsBin &bin) const; // Only for continuous variables
	virtual double lowerBoundToFloat(const StatsBin &bin) const; // Only for continuous variables
	virtual double upperBoundToFloat(const StatsBin &bin) const; // Only for continuous variables
	virtual bool preferBin(const StatsBin &bin) const; // Prefer to show this bins tick if bins are omitted. Default to true.

	// Only for continuous and numeric variables
	// Note: this will crash with an exception if passed incompatible bins!
	virtual std::vector<StatsBinPtr> bins_between(const StatsBin &bin1, const StatsBin &bin2) const;
};

// A scatter item is two values and a dive
struct StatsScatterItem {
	double x, y;
	dive *d;
};

struct StatsVariable {
	enum class Type {
		Discrete,
		Continuous,
		Numeric
	};

	virtual ~StatsVariable();
	virtual Type type() const = 0;
	virtual QString name() const = 0;
	virtual QString unitSymbol() const; // For numeric variables - by default returns empty string
	virtual int decimals() const; // For numeric variables: numbers of decimals to display on axes. Defaults to 0.
	virtual std::vector<const StatsBinner *> binners() const = 0; // Note: may depend on current locale!
	virtual QString diveCategories(const dive *d) const; // Only for discrete variables
	std::vector<StatsBinQuartiles> bin_quartiles(const StatsBinner &binner, const std::vector<dive *> &dives, bool fill_empty) const;
	std::vector<StatsBinOp> bin_operations(const StatsBinner &binner, const std::vector<dive *> &dives, bool fill_empty) const;
	std::vector<StatsBinValues> bin_values(const StatsBinner &binner, const std::vector<dive *> &dives, bool fill_empty) const;
	const StatsBinner *getBinner(int idx) const; // Handles out of bounds gracefully (returns first binner)
	QString nameWithUnit() const;
	QString nameWithBinnerUnit(const StatsBinner &) const;
	virtual std::vector<StatsOperation> supportedOperations() const; // Only for numeric variables
	QStringList supportedOperationNames() const; // Only for numeric variables
	StatsOperation idxToOperation(int idx) const;
	static QString operationName(StatsOperation);
	double mean(const std::vector<dive *> &dives) const; // Returns NaN for empty list
	static StatsQuartiles quartiles(const std::vector<StatsValue> &values); // Returns invalid quartiles for empty list
	StatsQuartiles quartiles(const std::vector<dive *> &dives) const; // Only for numeric variables
	std::vector<StatsValue> values(const std::vector<dive *> &dives) const; // Only for numeric variables
	QString valueWithUnit(const dive *d) const; // Only for numeric variables
	std::vector<StatsScatterItem> scatter(const StatsVariable &t2, const std::vector<dive *> &dives) const;
private:
	virtual double toFloat(const struct dive *d) const; // For numeric variables - if dive doesn't have that value, returns NaN
	StatsOperationResults applyOperations(const std::vector<dive *> &dives) const;
};

extern const std::vector<const StatsVariable *> stats_variables;

// Helper function for date-based variables
extern double date_to_double(int year, int month, int day);

#endif
