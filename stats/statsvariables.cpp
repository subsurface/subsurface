// SPDX-License-Identifier: GPL-2.0
#include "statsvariables.h"
#include "statstranslations.h"
#include "core/dive.h"
#include "core/divelog.h"
#include "core/divemode.h"
#include "core/divesite.h"
#include "core/gas.h"
#include "core/pref.h"
#include "core/qthelper.h" // for get_depth_unit() et al.
#include "core/string-format.h"
#include "core/tag.h"
#include "core/trip.h"
#include "core/subsurface-time.h"
#include <cmath>
#include <limits>
#include <QLocale>

static constexpr double NaN = std::numeric_limits<double>::quiet_NaN();

// Typedefs for year / quarter or month binners
using year_quarter = std::pair<unsigned short, unsigned short>;
using year_month = std::pair<unsigned short, unsigned short>;

// Small helper template: add an item to an unsorted vector, if its not already there.
template<typename T>
static void add_to_vector_unique(std::vector<T> &v, const T &item)
{
	if (std::find(v.begin(), v.end(), item) == v.end())
		v.push_back(item);
}

// Small helper: make a comma separeted list of a vector of QStrings
static QString join_strings(const std::vector<QString> &v)
{
	QString res;
	for (const QString &s: v) {
		if (!res.isEmpty())
			res += ", ";
		res += s;
	}
	return res;
}

// A wrapper around dive site that caches the name of the dive site
struct DiveSiteWrapper {
	const dive_site *ds;
	QString name;
	DiveSiteWrapper(const dive_site *ds) : ds(ds),
		name(ds ? ds->name : "")
	{
	}
	bool operator<(const DiveSiteWrapper &d2) const {
		if (ds == d2.ds)
			return false;
		if (int cmp = QString::compare(name, d2.name, Qt::CaseInsensitive))
			return cmp < 0;
		return ds < d2.ds; // This is just random, try something better.
	}
	bool operator==(const DiveSiteWrapper &d2) const {
		return ds == d2.ds;
	}
	bool operator!=(const DiveSiteWrapper &d2) const {
		return ds != d2.ds;
	}
	QString format() const {
		return ds ? name : StatsTranslations::tr("no divesite");
	}
};

// A wrapper around dive trips that caches the name and date of the trip and sorts by trip start date
struct TripWrapper {
	const dive_trip *t;
	QString name;
	timestamp_t date;
	TripWrapper(const dive_trip *t) : t(t),
		name(formatTripTitle(t)), // safe to pass null
		date(trip_date(t)) // safe to pass null
	{
	}
	bool operator<(const TripWrapper &t2) const {
		if (t == t2.t)
			return false;
		if (!t)
			return true;
		if (date == t2.date) {
			if (int cmp = QString::compare(name, t2.name, Qt::CaseInsensitive))
				return cmp < 0;
			return t < t2.t; // Basically random, but stable. What should we do if name and date are equal?
		}
		return date < t2.date;
	}
	bool operator==(const TripWrapper &t2) const {
		return t == t2.t;
	}
	bool operator!=(const TripWrapper &t2) const {
		return t != t2.t;
	}
	QString format() const {
		return t ? name : StatsTranslations::tr("no trip");
	}
};

// Note: usually I dislike functions defined inside class/struct
// declarations ("Java style"). However, for brevity this is done
// in this rather template-heavy source file more or less consistently.

// Templates to define invalid values for variables and test for said values.
// This is used by the binners: returning such a value means "ignore this dive".
template<typename T> T invalid_value();
template<> int invalid_value<int>()
{
	return std::numeric_limits<int>::max();
}
template<> double invalid_value<double>()
{
	return NaN;
}
template<> QString invalid_value<QString>()
{
	return QString();
}
template<> StatsQuartiles invalid_value<StatsQuartiles>()
{
	return { std::vector<dive *>(), NaN, NaN, NaN, NaN, NaN };
}

static bool is_invalid_value(int i)
{
	return i == std::numeric_limits<int>::max();
}

static bool is_invalid_value(double d)
{
	return std::isnan(d);
}

static bool is_invalid_value(const QString &s)
{
	return s.isEmpty();
}

// Currently, we don't support invalid dates - should we?
static bool is_invalid_value(const year_quarter &)
{
	return false;
}

static bool is_invalid_value(const DiveSiteWrapper &d)
{
	return !d.ds;
}

static bool is_invalid_value(const TripWrapper &t)
{
	return !t.t;
}

static bool is_invalid_value(const StatsOperationResults &res)
{
	return !res.isValid();
}

// Describes a gas-content bin. Since we consider O2 and He, these bins
// are effectively two-dimensional. However, they are given a linear order
// by sorting lexicographically.
// This is different from the general type of the gas, which is defined in
// gas.h as the gastype enum. The latter does not bin by fine-grained
// percentages.
struct gas_bin_t {
	// Depending on the gas content, we format the bins differently.
	// Eg. in the absence of helium and inreased oxygen, the bin is
	// formatted as "EAN32". The format is specified by the type enum.
	enum class Type {
		Air,
		Oxygen,
		EAN,
		Trimix
	} type;
	int o2, he;
	static gas_bin_t air() {
		return { Type::Air, 0, 0 };
	}
	static gas_bin_t oxygen() {
		return { Type::Oxygen, 0, 0 };
	}
	static gas_bin_t ean(int o2) {
		return { Type::EAN, o2, 0 };
	}
	static gas_bin_t trimix(int o2, int h2) {
		return { Type::Trimix, o2, h2 };
	}
};

// We never generate gas_bins of invalid gases.
static bool is_invalid_value(const gas_bin_t &)
{
	return false;
}

static bool is_invalid_value(const std::vector<StatsValue> &v)
{
	return v.empty();
}

static bool is_invalid_value(const StatsQuartiles &q)
{
	return q.dives.empty();
}

bool StatsQuartiles::isValid() const
{
	return !is_invalid_value(*this);
}

// Define an ordering for gas types
// invalid < air < ean (including oxygen) < trimix
// The latter two are sorted by (helium, oxygen)
// This is in analogy to the global get_dive_gas() function.
static bool operator<(const gas_bin_t &t1, const gas_bin_t &t2)
{
	if (t1.type != t2.type)
		return (int)t1.type < (int)t2.type;
	switch (t1.type) {
	default:
	case gas_bin_t::Type::Oxygen:
	case gas_bin_t::Type::Air:
		return false;
	case gas_bin_t::Type::EAN:
		return t1.o2 < t2.o2;
	case gas_bin_t::Type::Trimix:
		return std::tie(t1.o2, t1.he) < std::tie(t2.o2, t2.he);
	}
}

static bool operator==(const gas_bin_t &t1, const gas_bin_t &t2)
{
	return std::tie(t1.type, t1.o2, t1.he) ==
	       std::tie(t2.type, t2.o2, t2.he);
}

static bool operator!=(const gas_bin_t &t1, const gas_bin_t &t2)
{
	return !operator==(t1, t2);
}

// First, let's define the virtual destructors of our base classes
StatsBin::~StatsBin()
{
}

StatsBinner::~StatsBinner()
{
}

QString StatsBinner::unitSymbol() const
{
	return QString();
}

StatsVariable::~StatsVariable()
{
}

QString StatsBinner::name() const
{
	return QStringLiteral("N/A"); // Some dummy string that should never reach the UI
}

QString StatsBinner::formatWithUnit(const StatsBin &bin) const
{
	QString unit = unitSymbol();
	QString name = format(bin);
	return unit.isEmpty() ? std::move(name) : QStringLiteral("%1 %2").arg(name, unit);
}

QString StatsBinner::formatLowerBound(const StatsBin &bin) const
{
	return QStringLiteral("N/A"); // Some dummy string that should never reach the UI
}

QString StatsBinner::formatUpperBound(const StatsBin &bin) const
{
	return QStringLiteral("N/A"); // Some dummy string that should never reach the UI
}

double StatsBinner::lowerBoundToFloat(const StatsBin &bin) const
{
	return 0.0;
}

double StatsBinner::upperBoundToFloat(const StatsBin &bin) const
{
	return 0.0;
}

bool StatsBinner::preferBin(const StatsBin &bin) const
{
	return true;
}

// Default implementation for discrete variables: there are no bins between discrete bins.
std::vector<StatsBinPtr> StatsBinner::bins_between(const StatsBin &bin1, const StatsBin &bin2) const
{
	return {};
}

QString StatsVariable::unitSymbol() const
{
	return {};
}

QString StatsVariable::diveCategories(const dive *) const
{
	return QString();
}

int StatsVariable::decimals() const
{
	return 0;
}

double StatsVariable::toFloat(const dive *d) const
{
	return invalid_value<double>();
}

QString StatsVariable::nameWithUnit() const
{
	QString s = name();
	QString symb = unitSymbol();
	return symb.isEmpty() ? std::move(s) : QStringLiteral("%1 [%2]").arg(s, symb);
}

QString StatsVariable::nameWithBinnerUnit(const StatsBinner &binner) const
{
	QString s = name();
	QString symb = binner.unitSymbol();
	return symb.isEmpty() ? std::move(s) : QStringLiteral("%1 [%2]").arg(s, symb);
}

const StatsBinner *StatsVariable::getBinner(int idx) const
{
	std::vector<const StatsBinner *> b = binners();
	if (b.empty())
		return nullptr;
	return idx >= 0 && idx < (int)b.size() ? b[idx] : b[0];
}

std::vector<StatsOperation> StatsVariable::supportedOperations() const
{
	return {};
}

// Attn: The order must correspond to the StatsOperation enum
static const char *operation_names[] = {
	QT_TRANSLATE_NOOP("StatsTranslations", "Median"),
	QT_TRANSLATE_NOOP("StatsTranslations", "Mean"),
	QT_TRANSLATE_NOOP("StatsTranslations", "Time-weighted mean"),
	QT_TRANSLATE_NOOP("StatsTranslations", "Sum"),
	QT_TRANSLATE_NOOP("StatsTranslations", "Minimum"),
	QT_TRANSLATE_NOOP("StatsTranslations", "Maximum")
};

QStringList StatsVariable::supportedOperationNames() const
{
	std::vector<StatsOperation> ops = supportedOperations();
	QStringList res;
	res.reserve(ops.size());
	for (StatsOperation op: ops)
		res.push_back(operationName(op));
	return res;
}

StatsOperation StatsVariable::idxToOperation(int idx) const
{
	std::vector<StatsOperation> ops = supportedOperations();
	if (ops.empty()) {
		qWarning("Stats variable %s does not support operations", qPrintable(name()));
		return StatsOperation::Median; // oops!
	}
	return idx < 0 || idx >= (int)ops.size() ? ops[0] : ops[idx];
}

QString StatsVariable::operationName(StatsOperation op)
{
	int idx = (int)op;
	return idx < 0 || idx >= (int)std::size(operation_names) ? QString()
								 : operation_names[(int)op];
}

double StatsVariable::mean(const std::vector<dive *> &dives) const
{
	StatsOperationResults res = applyOperations(dives);
	return res.isValid() ? res.mean : invalid_value<double>();
}

std::vector<StatsValue> StatsVariable::values(const std::vector<dive *> &dives) const
{
	std::vector<StatsValue> vec;
	vec.reserve(dives.size());
	for (dive *d: dives) {
		double v = toFloat(d);
		if (!is_invalid_value(v))
			vec.push_back({ v, d });
	}
	std::sort(vec.begin(), vec.end(),
		  [](const StatsValue &v1, const StatsValue &v2)
		  { return v1.v < v2.v; });
	return vec;
}

QString StatsVariable::valueWithUnit(const dive *d) const
{
	QLocale loc;
	double v = toFloat(d);
	if (is_invalid_value(v))
		return QStringLiteral("-");
	return QString("%1 %2").arg(loc.toString(v, 'f', decimals()),
				    unitSymbol());
}

// Small helper to calculate quartiles - section of intervals of
// two consecutive elements in a vector. It's not strictly correct
// to interpolate linearly. However, on the one hand we don't know
// the actual distribution, on the other hand for a discrete
// distribution the quartiles are ranges. So what should we do?
static double q1(const StatsValue *v)
{
	return (3.0*v[0].v + v[1].v) / 4.0;
}
static double q2(const StatsValue *v)
{
	return (v[0].v + v[1].v) / 2.0;
}
static double q3(const StatsValue *v)
{
	return (v[0].v + 3.0*v[1].v) / 4.0;
}

StatsQuartiles StatsVariable::quartiles(const std::vector<dive *> &dives) const
{
	return quartiles(values(dives));
}

// This expects the value vector to be sorted!
StatsQuartiles StatsVariable::quartiles(const std::vector<StatsValue> &vec)
{
	int s = (int)vec.size();
	if (s <= 0)
		return invalid_value<StatsQuartiles>();
	std::vector<dive *> dives;
	dives.reserve(vec.size());
	for (const auto &[v, d]: vec)
		dives.push_back(d);

	switch (s % 4) {
	default:
		// gcc doesn't recognize that we catch all possible values. disappointing.
	case 0:
		return { std::move(dives), vec[0].v, q3(&vec[s/4 - 1]), q2(&vec[s/2 - 1]), q1(&vec[s - s/4 - 1]), vec[s - 1].v };
	case 1:
		return { std::move(dives), vec[0].v, vec[s/4].v, vec[s/2].v, vec[s - s/4 - 1].v, vec[s - 1].v };
	case 2:
		return { std::move(dives), vec[0].v, q1(&vec[s/4]), q2(&vec[s/2 - 1]), q3(&vec[s - s/4 - 2]), vec[s - 1].v };
	case 3:
		return { std::move(dives), vec[0].v, q2(&vec[s/4]), vec[s/2].v, q2(&vec[s - s/4 - 2]), vec[s - 1].v };
	}
}

StatsOperationResults StatsVariable::applyOperations(const std::vector<dive *> &dives) const
{
	StatsOperationResults res;
	std::vector<StatsValue> val = values(dives);

	double sumTime = 0.0;
	res.dives.reserve(val.size());
	res.median = quartiles(val).q2;

	if (val.empty())
		return res;

	res.min = std::numeric_limits<double>::max();
	res.max = std::numeric_limits<double>::lowest();
	for (auto [v, d]: val) {
		res.dives.push_back(d);
		res.sum += v;
		res.mean += v;
		sumTime += d->duration.seconds;
		res.timeWeightedMean += v * d->duration.seconds;
		if (v < res.min)
			res.min = v;
		if (v > res.max)
			res.max = v;
	}

	res.mean /= val.size();
	res.timeWeightedMean /= sumTime;
	return res;
}

StatsOperationResults::StatsOperationResults() :
	median(0.0), mean(0.0), timeWeightedMean(0.0), sum(0.0), min(0.0), max(0.0)
{
}

bool StatsOperationResults::isValid() const
{
	return !dives.empty();
}

double StatsOperationResults::get(StatsOperation op) const
{
	switch (op) {
	case StatsOperation::Median: return median;
	case StatsOperation::Mean: return mean;
	case StatsOperation::TimeWeightedMean: return timeWeightedMean;
	case StatsOperation::Sum: return sum;
	case StatsOperation::Min: return min;
	case StatsOperation::Max: return max;
	case StatsOperation::Invalid:
	default: return invalid_value<double>();
	}
}

std::vector<StatsScatterItem> StatsVariable::scatter(const StatsVariable &t2, const std::vector<dive *> &dives) const
{
	std::vector<StatsScatterItem> res;
	res.reserve(dives.size());
	for (dive *d: dives) {
		double v1 = toFloat(d);
		double v2 = t2.toFloat(d);
		if (is_invalid_value(v1) || is_invalid_value(v2))
			continue;
		res.push_back({ v1, v2, d });
	}
	std::sort(res.begin(), res.end(),
		  [](const StatsScatterItem &i1, const StatsScatterItem &i2)
		  { return std::tie(i1.x, i1.y) < std::tie(i2.x, i2.y); }); // use std::tie() for lexicographical comparison
	return res;
}

template <typename T, typename DivesToValueFunc>
std::vector<StatsBinValue<T>> bin_convert(const StatsVariable &variable, const StatsBinner &binner, const std::vector<dive *> &dives,
					  bool fill_empty, DivesToValueFunc func)
{
	std::vector<StatsBinDives> bin_dives = binner.bin_dives(dives, fill_empty);
	std::vector<StatsBinValue<T>> res;
	res.reserve(bin_dives.size());
	for (auto &[bin, dives]: bin_dives) {
		T v = func(dives);
		if (is_invalid_value(v) && (res.empty() || !fill_empty))
			continue;
		res.push_back({ std::move(bin), std::move(v) });
	}
	if (res.empty())
		return res;

	// Check if we added invalid items at the end.
	// Note: we added at least one valid item.
	auto it = res.end() - 1;
	while (it != res.begin() && is_invalid_value(it->value))
		--it;
	res.erase(it + 1, res.end());
	return res;
}

std::vector<StatsBinQuartiles> StatsVariable::bin_quartiles(const StatsBinner &binner, const std::vector<dive *> &dives, bool fill_empty) const
{
	return bin_convert<StatsQuartiles>(*this, binner, dives, fill_empty,
					   [this](const std::vector<dive *> &d) { return quartiles(d); });
}

std::vector<StatsBinOp> StatsVariable::bin_operations(const StatsBinner &binner, const std::vector<dive *> &dives, bool fill_empty) const
{
	return bin_convert<StatsOperationResults>(*this, binner, dives, fill_empty,
						  [this](const std::vector<dive *> &d) { return applyOperations(d); });
}

std::vector<StatsBinValues> StatsVariable::bin_values(const StatsBinner &binner, const std::vector<dive *> &dives, bool fill_empty) const
{
	return bin_convert<std::vector<StatsValue>>(*this, binner, dives, fill_empty,
						    [this](const std::vector<dive *> &d) { return values(d); });
}

// Silly template, which spares us defining type() member functions.
template<StatsVariable::Type t>
struct StatsVariableTemplate : public StatsVariable {
	Type type() const override { return t; }
};

// A simple bin that is based on copyable value and can be initialized from
// that value. This template spares us from writing one-line constructors.
template<typename Type>
struct SimpleBin : public StatsBin {
	Type value;
	SimpleBin(const Type &v) : value(v) { }

	// This must not be called for different types. It will crash with an exception.
	bool operator<(StatsBin &b) const {
		return value < dynamic_cast<SimpleBin &>(b).value;
	}

	bool operator==(StatsBin &b) const {
		return value == dynamic_cast<SimpleBin &>(b).value;
	}
};
using IntBin = SimpleBin<int>;
using StringBin = SimpleBin<QString>;
using GasTypeBin = SimpleBin<gas_bin_t>;

// A general binner template that works on trivial bins that are based
// on a type that is equality and less-than comparable. The derived class
// must possess:
//  - A to_bin_value() function that turns a dive into a value from
//    which the bins can be constructed.
//  - A lowerBoundToFloatBase() function that turns the value form
//    into a double which is understood by the StatsVariable.
// The bins must possess:
//  - A member variable "value" of the type it is constructed with.
// Note: this uses the curiously recurring template pattern, which I
// dislike, but it is the easiest thing for now.
template<typename Binner, typename Bin>
struct SimpleBinner : public StatsBinner {
public:
	using Type = decltype(Bin::value);
	std::vector<StatsBinDives> bin_dives(const std::vector<dive *> &dives, bool fill_empty) const override;
	const Binner &derived() const {
		return static_cast<const Binner &>(*this);
	}
	const Bin &derived_bin(const StatsBin &bin) const {
		return dynamic_cast<const Bin &>(bin);
	}
};

// Wrapper around std::lower_bound that searches for a value in a
// vector of pairs. Comparison is made with the first element of the pair.
// std::lower_bound does a binary search and this is used to keep a
// vector in ascending order.
template<typename T1, typename T2>
auto pair_lower_bound(std::vector<std::pair<T1, T2>> &v, const T1 &value)
{
	return std::lower_bound(v.begin(), v.end(), value,
				[] (const std::pair<T1, T2> &entry, const T1 &value) {
					return entry.first < value;
				});
}

// Register a dive in a (bin_value, value) pair. The second value can be
// anything, for example a count or a list of dives. If the bin does not
// exist, it is created. The add_dive_func() function increase the second
// value accordingly.
template<typename BinValueType, typename ValueType, typename AddDiveFunc>
void register_bin_value(std::vector<std::pair<BinValueType, ValueType>> &v,
			const BinValueType &bin,
			AddDiveFunc add_dive_func)
{
	// Does that value already exist?
	auto it = pair_lower_bound(v, bin);
	if (it == v.end() || it->first != bin)
		it = v.insert(it, { bin, ValueType() });	// Bin does not exist -> insert at proper location.
	add_dive_func(it->second);				// Register dive
}

// Turn a (bin-value, value)-pair vector into a (bin, value)-pair vector.
// The values are moved out of the first vectors.
// If fill_empty is true, missing bins will be completed with a default constructed
// value.
template<typename Bin, typename Binner, typename BinValueType, typename ValueType>
std::vector<StatsBinValue<ValueType>>
value_vector_to_bin_vector(const Binner &binner, std::vector<std::pair<BinValueType, ValueType>> &value_bins,
						      bool fill_empty)
{
	std::vector<StatsBinValue<ValueType>> res;
	res.reserve(value_bins.size());
	for (const auto &[bin_value, value]: value_bins) {
		StatsBinPtr b = std::make_unique<Bin>(bin_value);
		if (fill_empty && !res.empty()) {
			// Add empty bins, if any
			for (StatsBinPtr &bin: binner.bins_between(*res.back().bin, *b))
				res.push_back({ std::move(bin), ValueType() });
		}
		res.push_back({ std::move(b), std::move(value)});
	}
	return res;
}

template<typename Binner, typename Bin>
std::vector<StatsBinDives> SimpleBinner<Binner, Bin>::bin_dives(const std::vector<dive *> &dives, bool fill_empty) const
{
	// First, collect a value / dives vector and then produce the final vector
	// out of that. I wonder if that is premature optimization?
	using Pair = std::pair<Type, std::vector<dive *>>;
	std::vector<Pair> value_bins;
	for (dive *d: dives) {
		Type value = derived().to_bin_value(d);
		if (is_invalid_value(value))
			continue;
		register_bin_value(value_bins, value,
				   [d](std::vector<dive *> &v) { v.push_back(d); });
	}

	// Now, turn that into our result array with allocated bin objects.
	return value_vector_to_bin_vector<Bin>(*this, value_bins, fill_empty);
}

// A simple binner (see above) that works on continuous (or numeric) variables
// and can return bin-ranges. The binner must implement an inc() function
// that turns a bin into the next-higher bin.
template<typename Binner, typename Bin>
struct SimpleContinuousBinner : public SimpleBinner<Binner, Bin>
{
	using SimpleBinner<Binner, Bin>::derived;
	std::vector<StatsBinPtr> bins_between(const StatsBin &bin1, const StatsBin &bin2) const override;

	// By default the value gives the lower bound, so the format is the same
	QString formatLowerBound(const StatsBin &bin) const override {
		return derived().format(bin);
	}

	// For the upper bound, simply go to the next bin
	QString formatUpperBound(const StatsBin &bin) const override {
		Bin b = SimpleBinner<Binner,Bin>::derived_bin(bin);
		derived().inc(b);
		return formatLowerBound(b);
	}

	// Cast to base value type so that the derived class doesn't have to do it
	double lowerBoundToFloat(const StatsBin &bin) const override {
		const Bin &b = SimpleBinner<Binner,Bin>::derived_bin(bin);
		return derived().lowerBoundToFloatBase(b.value);
	}

	// For the upper bound, simply go to the next bin
	double upperBoundToFloat(const StatsBin &bin) const override {
		Bin b = SimpleBinner<Binner,Bin>::derived_bin(bin);
		derived().inc(b);
		return derived().lowerBoundToFloatBase(b.value);
	}
};

// A continuous binner, where the bin is based on an integer value
// and subsequent bins are adjacent integers.
template<typename Binner, typename Bin>
struct IntBinner : public SimpleContinuousBinner<Binner, Bin>
{
	void inc(Bin &bin) const {
		++bin.value;
	}
};

// An integer based binner, where each bin represents an integer
// range with a fixed size.
template<typename Binner, typename Bin>
struct IntRangeBinner : public IntBinner<Binner, Bin> {
	int bin_size;
	IntRangeBinner(int size)
		: bin_size(size)
	{
	}
	QString format(const StatsBin &bin) const override {
		int value = IntBinner<Binner, Bin>::derived_bin(bin).value;
		QLocale loc;
		return StatsTranslations::tr("%1–%2").arg(loc.toString(value * bin_size),
							  loc.toString((value + 1) * bin_size));
	}
	QString formatLowerBound(const StatsBin &bin) const override {
		int value = IntBinner<Binner, Bin>::derived_bin(bin).value;
		return QStringLiteral("%L1").arg(value * bin_size);
	}
	double lowerBoundToFloatBase(int value) const {
		return static_cast<double>(value * bin_size);
	}
};

template<typename Binner, typename Bin>
std::vector<StatsBinPtr> SimpleContinuousBinner<Binner, Bin>::bins_between(const StatsBin &bin1, const StatsBin &bin2) const
{
	const Bin &b1 = SimpleBinner<Binner,Bin>::derived_bin(bin1);
	const Bin &b2 = SimpleBinner<Binner,Bin>::derived_bin(bin2);
	std::vector<StatsBinPtr> res;
	Bin act = b1;
	derived().inc(act);
	while (act.value < b2.value) {
		res.push_back(std::make_unique<Bin>(act));
		derived().inc(act);
	}
	return res;
}

// A binner template for discrete variables that where each dive can belong to
// multiple bins. The bin-type must be less-than comparable. The derived class
// must possess:
//  - A to_bin_values() function that turns a dive into a value from
//    which the bins can be constructed.
// The bins must possess:
//  - A member variable "value" of the type it is constructed with.
template<typename Binner, typename Bin>
struct MultiBinner : public StatsBinner {
public:
	using Type = decltype(Bin::value);
	std::vector<StatsBinDives> bin_dives(const std::vector<dive *> &dives, bool fill_empty) const override;
	const Binner &derived() const {
		return static_cast<const Binner &>(*this);
	}
	const Bin &derived_bin(const StatsBin &bin) const {
		return dynamic_cast<const Bin &>(bin);
	}
};

template<typename Binner, typename Bin>
std::vector<StatsBinDives> MultiBinner<Binner, Bin>::bin_dives(const std::vector<dive *> &dives, bool) const
{
	// First, collect a value / dives vector and then produce the final vector
	// out of that. I wonder if that is premature optimization?
	using Pair = std::pair<Type, std::vector<dive *>>;
	std::vector<Pair> value_bins;
	for (dive *d: dives) {
		for (const Type &val: derived().to_bin_values(d)) {
			if (is_invalid_value(val))
				continue;
			register_bin_value(value_bins, val,
					   [d](std::vector<dive *> &v) { v.push_back(d); });
		}
	}

	// Now, turn that into our result array with allocated bin objects.
	return value_vector_to_bin_vector<Bin>(*this, value_bins, false);
}

// A binner that works on string-based bins whereby each dive can
// produce multiple strings (e.g. dive buddies). The binner must
// feature a to_bin_values() function that produces a vector of
// QStrings and bins that can be constructed from QStrings.
// Other than that, see SimpleBinner.
template<typename Binner, typename Bin>
struct StringBinner : public MultiBinner<Binner, Bin> {
public:
	QString format(const StatsBin &bin) const override {
		return dynamic_cast<const Bin &>(bin).value;
	}
};

// ============ The date of the dive by year, quarter or month ============
// (Note that calendar week is defined differently in different parts of the world and therefore omitted for now)

double date_to_double(int year, int month, int day)
{
	struct tm tm = { 0 };
	tm.tm_year = year;
	tm.tm_mon = month;
	tm.tm_mday = day;
	timestamp_t t = utc_mktime(&tm);
	return t / 86400.0; // Turn seconds since 1970 to days since 1970, if that makes sense...?
}

struct DateYearBinner : public IntBinner<DateYearBinner, IntBin> {
	QString name() const override {
		return StatsTranslations::tr("Yearly");
	}
	QString format(const StatsBin &bin) const override {
		return QString::number(derived_bin(bin).value);
	}
	int to_bin_value(const dive *d) const {
		return utc_year(d->when);
	}
	double lowerBoundToFloatBase(int year) const {
		return date_to_double(year, 0, 0);
	}
};

using DateQuarterBin = SimpleBin<year_quarter>;

struct DateQuarterBinner : public SimpleContinuousBinner<DateQuarterBinner, DateQuarterBin> {
	QString name() const override {
		return StatsTranslations::tr("Quarterly");
	}
	QString format(const StatsBin &bin) const override {
		year_quarter value = derived_bin(bin).value;
		return StatsTranslations::tr("%1 Q%2").arg(QString::number(value.first),
							   QString::number(value.second));
	}
	// As histogram axis: show full year for new years and then Q2, Q3, Q4.
	QString formatLowerBound(const StatsBin &bin) const override {
		year_quarter value = derived_bin(bin).value;
		return value.second == 1
			? QString::number(value.first)
			: StatsTranslations::tr("Q%1").arg(QString::number(value.second));
	}
	double lowerBoundToFloatBase(year_quarter value) const {
		return date_to_double(value.first, (value.second - 1) * 3, 0);
	}
	// Prefer bins that show full years
	bool preferBin(const StatsBin &bin) const override {
		year_quarter value = derived_bin(bin).value;
		return value.second == 1;
	}
	year_quarter to_bin_value(const dive *d) const {
		struct tm tm;
		utc_mkdate(d->when, &tm);

		int year = tm.tm_year;
		switch (tm.tm_mon) {
		case 0 ... 2: return { year, 1 };
		case 3 ... 5: return { year, 2 };
		case 6 ... 8: return { year, 3 };
		default:      return { year, 4 };
		}
	}
	void inc(DateQuarterBin &bin) const {
		if (++bin.value.second > 4) {
			bin.value.second = 1;
			++bin.value.first;
		}
	}
};

using DateMonthBin = SimpleBin<year_month>;

struct DateMonthBinner : public SimpleContinuousBinner<DateMonthBinner, DateMonthBin> {
	QString name() const override {
		return StatsTranslations::tr("Monthly");
	}
	QString format(const StatsBin &bin) const override {
		year_month value = derived_bin(bin).value;
		return QString("%1 %2").arg(monthname(value.second), QString::number(value.first));
	}
	// In histograms, output year for full years, month otherwise
	QString formatLowerBound(const StatsBin &bin) const override {
		year_month value = derived_bin(bin).value;
		return value.second == 0 ? QString::number(value.first)
					 : QString(monthname(value.second));
	}
	double lowerBoundToFloatBase(year_quarter value) const {
		return date_to_double(value.first, value.second, 0);
	}
	// Prefer bins that show full years
	bool preferBin(const StatsBin &bin) const override {
		year_month value = derived_bin(bin).value;
		return value.second == 0;
	}
	year_month to_bin_value(const dive *d) const {
		struct tm tm;
		utc_mkdate(d->when, &tm);
		return { tm.tm_year, tm.tm_mon };
	}
	void inc(DateMonthBin &bin) const {
		if (++bin.value.second > 11) {
			bin.value.second = 0;
			++bin.value.first;
		}
	}
};

static DateYearBinner date_year_binner;
static DateQuarterBinner date_quarter_binner;
static DateMonthBinner date_month_binner;
struct DateVariable : public StatsVariableTemplate<StatsVariable::Type::Continuous> {
	QString name() const {
		return StatsTranslations::tr("Date");
	}
	double toFloat(const dive *d) const override {
		return d->when / 86400.0;
	}
	std::vector<const StatsBinner *> binners() const override {
		return { &date_year_binner, &date_quarter_binner, &date_month_binner };
	}
};

// ============ Dive depth (max and mean), binned in 5, 10, 20 m or 15, 30, 60 ft bins ============

static int dive_depth_in_mm(const dive *d, bool mean)
{
	return mean ? d->meandepth.mm : d->maxdepth.mm;
}

struct DepthBinner : public IntRangeBinner<DepthBinner, IntBin> {
	bool metric;
	bool mean;
	DepthBinner(int bin_size, bool metric, bool mean) :
		IntRangeBinner(bin_size), metric(metric), mean(mean)
	{
	}
	QString name() const override {
		QLocale loc;
		return StatsTranslations::tr("in %1 %2 steps").arg(loc.toString(bin_size),
								   get_depth_unit(metric));
	}
	QString unitSymbol() const override {
		return get_depth_unit(metric);
	}
	int to_bin_value(const dive *d) const {
		int depth = dive_depth_in_mm(d, mean);
		return metric ? depth / 1000 / bin_size
			      : lrint(mm_to_feet(depth)) / bin_size;
	}
};

struct DepthVariableBase : public StatsVariableTemplate<StatsVariable::Type::Numeric> {
	bool mean;
	DepthBinner meter5, meter10, meter20;
	DepthBinner feet15, feet30, feet60;
	DepthVariableBase(bool mean) :
		mean(mean),
		meter5(5, true, mean), meter10(10, true, mean), meter20(20, true, mean),
		feet15(15, false, mean), feet30(30, false, mean), feet60(60, false, mean)
	{
	}
	QString unitSymbol() const override {
		return get_depth_unit();
	}
	int decimals() const override {
		return 1;
	}
	std::vector<const StatsBinner *> binners() const override {
		if (prefs.units.length == units::METERS)
			return { &meter5, &meter10, &meter20 };
		else
			return { &feet15, &feet30, &feet60 };
	}
	double toFloat(const dive *d) const override {
		int depth = dive_depth_in_mm(d, mean);
		return prefs.units.length == units::METERS ? depth / 1000.0
							   : mm_to_feet(depth);
	}
};

struct MaxDepthVariable : public DepthVariableBase {
	MaxDepthVariable() : DepthVariableBase(false) {
	}
	QString name() const override {
		return StatsTranslations::tr("Max. Depth");
	}
	std::vector<StatsOperation> supportedOperations() const override {
		return { StatsOperation::Median, StatsOperation::Mean, StatsOperation::Sum, StatsOperation::Min, StatsOperation::Max };
	}
};

struct MeanDepthVariable : public DepthVariableBase {
	MeanDepthVariable() : DepthVariableBase(true)
	{
	}
	QString name() const override {
		return StatsTranslations::tr("Mean Depth");
	}
	std::vector<StatsOperation> supportedOperations() const override {
		return { StatsOperation::Median, StatsOperation::Mean, StatsOperation::TimeWeightedMean, StatsOperation::Min, StatsOperation::Max };
	}
};

// ============ Bottom time, binned in 5, 10, 30 min or 1 h bins ============

struct MinuteBinner : public IntRangeBinner<MinuteBinner, IntBin> {
	using IntRangeBinner::IntRangeBinner;
	QString name() const override {
		return StatsTranslations::tr("in %1 min steps").arg(bin_size);
	}
	QString unitSymbol() const override {
		return StatsTranslations::tr("min");
	}
	int to_bin_value(const dive *d) const {
		return d->duration.seconds / 60 / bin_size;
	}
};

struct HourBinner : public IntBinner<HourBinner, IntBin> {
	QString name() const override {
		return StatsTranslations::tr("in hours");
	}
	QString format(const StatsBin &bin) const override {
		return QString::number(derived_bin(bin).value);
	}
	QString unitSymbol() const override {
		return StatsTranslations::tr("h");
	}
	int to_bin_value(const dive *d) const {
		return d->duration.seconds / 3600;
	}
	double lowerBoundToFloatBase(int hour) const {
		return static_cast<double>(hour);
	}
};

static MinuteBinner minute_binner5(5);
static MinuteBinner minute_binner10(10);
static MinuteBinner minute_binner30(30);
static HourBinner hour_binner;
struct DurationVariable : public StatsVariableTemplate<StatsVariable::Type::Numeric> {
	QString name() const override {
		return StatsTranslations::tr("Duration");
	}
	QString unitSymbol() const override {
		return StatsTranslations::tr("min");
	}
	int decimals() const override {
		return 0;
	}
	std::vector<const StatsBinner *> binners() const override {
		return { &minute_binner5, &minute_binner10, &minute_binner30, &hour_binner };
	}
	double toFloat(const dive *d) const override {
		return d->duration.seconds / 60.0;
	}
	std::vector<StatsOperation> supportedOperations() const override {
		return { StatsOperation::Median, StatsOperation::Mean, StatsOperation::Sum, StatsOperation::Min, StatsOperation::Max };
	}
};

// ============ SAC, binned in 2, 5, 10 l/min or 0.1, 0.2, 0.4, 0.8 cuft/min bins ============

struct MetricSACBinner : public IntRangeBinner<MetricSACBinner, IntBin> {
	using IntRangeBinner::IntRangeBinner;
	QString name() const override {
		QLocale loc;
		return StatsTranslations::tr("in %1 %2/min steps").arg(loc.toString(bin_size),
								       get_volume_unit());
	}
	QString unitSymbol() const override {
		return get_volume_unit(true) + StatsTranslations::tr("/min");
	}
	int to_bin_value(const dive *d) const {
		if (d->sac <= 0)
			return invalid_value<int>();
		return d->sac / 1000 / bin_size;
	}
};

// "Imperial" SACs are annoying, since we have to bin to sub-integer precision.
// We store cuft * 100 as an integer, to avoid troubles with floating point semantics.

struct ImperialSACBinner : public IntBinner<ImperialSACBinner, IntBin> {
	int bin_size;
	ImperialSACBinner(double size)
		: bin_size(lrint(size * 100.0))
	{
	}
	QString name() const override {
		QLocale loc;
		return StatsTranslations::tr("in %1 %2/min steps").arg(loc.toString(bin_size / 100.0, 'f', 2),
								       get_volume_unit());
	}
	QString format(const StatsBin &bin) const override {
		int value = derived_bin(bin).value;
		QLocale loc;
		return StatsTranslations::tr("%1–%2").arg(loc.toString((value * bin_size) / 100.0, 'f', 2),
							  loc.toString(((value + 1) * bin_size) / 100.0, 'f', 2));
	}
	QString unitSymbol() const override {
		return get_volume_unit(false) + StatsTranslations::tr("/min");
	}
	QString formatLowerBound(const StatsBin &bin) const override {
		int value = derived_bin(bin).value;
		return QStringLiteral("%L1").arg((value * bin_size) / 100.0, 0, 'f', 2);
	}
	double lowerBoundToFloatBase(int value) const {
		return static_cast<double>((value * bin_size) / 100.0);
	}
	int to_bin_value(const dive *d) const {
		if (d->sac <= 0)
			return invalid_value<int>();
		return lrint(ml_to_cuft(d->sac) * 100.0) / bin_size;
	}
};

MetricSACBinner metric_sac_binner2(2);
MetricSACBinner metric_sac_binner5(5);
MetricSACBinner metric_sac_binner10(10);
ImperialSACBinner imperial_sac_binner1(0.1);
ImperialSACBinner imperial_sac_binner2(0.2);
ImperialSACBinner imperial_sac_binner4(0.4);
ImperialSACBinner imperial_sac_binner8(0.8);

struct SACVariable : public StatsVariableTemplate<StatsVariable::Type::Numeric> {
	QString name() const override {
		return StatsTranslations::tr("SAC");
	}
	QString unitSymbol() const override {
		return get_volume_unit() + StatsTranslations::tr("/min");
	}
	int decimals() const override {
		return prefs.units.volume == units::LITER ? 0 : 2;
	}
	std::vector<const StatsBinner *> binners() const override {
		if (prefs.units.volume == units::LITER)
			return { &metric_sac_binner2, &metric_sac_binner5, &metric_sac_binner10 };
		else
			return { &imperial_sac_binner1, &imperial_sac_binner2, &imperial_sac_binner4, &imperial_sac_binner8 };
	}
	double toFloat(const dive *d) const override {
		if (d->sac <= 0)
			return invalid_value<double>();
		return prefs.units.volume == units::LITER ? d->sac / 1000.0 :
							    ml_to_cuft(d->sac);
	}
	std::vector<StatsOperation> supportedOperations() const override {
		return { StatsOperation::Median, StatsOperation::Mean, StatsOperation::TimeWeightedMean, StatsOperation::Min, StatsOperation::Max };
	}
};

// ============ Water and air temperature, binned in 2, 5, 10, 20 °C/°F bins ============

struct TemperatureBinner : public IntRangeBinner<TemperatureBinner, IntBin> {
	bool air;
	bool metric;
	TemperatureBinner(int bin_size, bool air, bool metric) :
		IntRangeBinner(bin_size),
		air(air),
		metric(metric)
	{
	}
	QString name() const override {
		QLocale loc;
		return StatsTranslations::tr("in %1 %2 steps").arg(loc.toString(bin_size),
								   get_temp_unit(metric));
	}
	QString unitSymbol() const override {
		return get_temp_unit(metric);
	}
	int to_bin_value(const struct dive *d) const {
		temperature_t t = air ? d->airtemp : d->watertemp;
		if (t.mkelvin <= 0)
			return invalid_value<int>();
		int temp = metric ?  static_cast<int>(mkelvin_to_C(t.mkelvin))
				  :  static_cast<int>(mkelvin_to_F(t.mkelvin));
		return temp / bin_size;
	}
};

struct TemperatureVariable : public StatsVariableTemplate<StatsVariable::Type::Numeric> {
	TemperatureBinner bin2C, bin5C, bin10C, bin20C;
	TemperatureBinner bin2F, bin5F, bin10F, bin20F;
	TemperatureVariable(bool air) :
		bin2C(2, air, true), bin5C(5, air, true), bin10C(10, air, true), bin20C(20, air, true),
		bin2F(2, air, false), bin5F(5, air, false), bin10F(10, air, false), bin20F(20, air, false)
	{
	}
	QString unitSymbol() const override {
		return get_temp_unit();
	}
	int decimals() const override {
		return 1;
	}
	double tempToFloat(temperature_t t) const {
		if (t.mkelvin <= 0)
			return invalid_value<double>();
		return prefs.units.temperature == units::CELSIUS ?
				mkelvin_to_C(t.mkelvin) : mkelvin_to_F(t.mkelvin);
	}
	std::vector<const StatsBinner *> binners() const override {
		if (prefs.units.temperature == units::CELSIUS)
			return { &bin2C, &bin5C, &bin10C, &bin20C };
		else
			return { &bin2F, &bin5F, &bin10F, &bin20F };
	}
	std::vector<StatsOperation> supportedOperations() const override {
		return { StatsOperation::Median, StatsOperation::Mean, StatsOperation::TimeWeightedMean, StatsOperation::Min, StatsOperation::Max };
	}
};

struct WaterTemperatureVariable : TemperatureVariable {
	WaterTemperatureVariable() : TemperatureVariable(false)
	{
	}
	QString name() const override {
		return StatsTranslations::tr("Water temperature");
	}
	double toFloat(const dive *d) const override {
		return tempToFloat(d->watertemp);
	}
};

struct AirTemperatureVariable : TemperatureVariable {
	AirTemperatureVariable() : TemperatureVariable(true)
	{
	}
	QString name() const override {
		return StatsTranslations::tr("Air temperature");
	}
	double toFloat(const dive *d) const override {
		return tempToFloat(d->airtemp);
	}
};

// ============ Weight, binned in 1, 2, 5, 10 kg or 2, 4, 10 or 20 lbs bins ============

struct WeightBinner : public IntRangeBinner<WeightBinner, IntBin> {
	bool metric;
	WeightBinner(int bin_size, bool metric) : IntRangeBinner(bin_size), metric(metric)
	{
	}
	QString name() const override {
		QLocale loc;
		return StatsTranslations::tr("in %1 %2 steps").arg(loc.toString(bin_size),
								   get_weight_unit(metric));
	}
	QString unitSymbol() const override {
		return get_weight_unit(metric);
	}
	int to_bin_value(const dive *d) const {
		return metric ? total_weight(d) / 1000 / bin_size
			      : lrint(grams_to_lbs(total_weight(d))) / bin_size;
	}
};

static WeightBinner weight_binner_1kg(1, true);
static WeightBinner weight_binner_2kg(2, true);
static WeightBinner weight_binner_5kg(5, true);
static WeightBinner weight_binner_10kg(10, true);
static WeightBinner weight_binner_2lbs(2, false);
static WeightBinner weight_binner_5lbs(4, false);
static WeightBinner weight_binner_10lbs(10, false);
static WeightBinner weight_binner_20lbs(20, false);

struct WeightVariable : public StatsVariableTemplate<StatsVariable::Type::Numeric> {
	QString name() const override {
		return StatsTranslations::tr("Weight");
	}
	QString unitSymbol() const override {
		return get_weight_unit();
	}
	int decimals() const override {
		return 1;
	}
	std::vector<const StatsBinner *> binners() const override {
		if (prefs.units.weight == units::KG)
			return { &weight_binner_1kg, &weight_binner_2kg, &weight_binner_5kg, &weight_binner_10kg };
		else
			return { &weight_binner_2lbs, &weight_binner_5lbs, &weight_binner_10lbs, &weight_binner_20lbs };
	}
	double toFloat(const dive *d) const override {
		return prefs.units.weight == units::KG ? total_weight(d) / 1000.0
						       : grams_to_lbs(total_weight(d));
	}
	std::vector<StatsOperation> supportedOperations() const override {
		return { StatsOperation::Median, StatsOperation::Mean, StatsOperation::Sum, StatsOperation::Min, StatsOperation::Max };
	}
};

// ============ Dive number ============

// Binning dive numbers can't use the IntRangeBinner,
// because dive numbers start at 1, not 0! Since 0 means
// "no number", these dives aren't registered.
struct DiveNrBinner : public IntBinner<DiveNrBinner, IntBin> {
	int bin_size;
	DiveNrBinner(int bin_size) : bin_size(bin_size)
	{
	}
	QString format(const StatsBin &bin) const override {
		int value = derived_bin(bin).value;
		QLocale loc;
		return StatsTranslations::tr("%1–%2").arg(loc.toString(value * bin_size + 1),
							  loc.toString((value + 1) * bin_size));
	}
	QString formatLowerBound(const StatsBin &bin) const override {
		int value = derived_bin(bin).value;
		return QStringLiteral("%L1").arg(value * bin_size + 1);
	}
	double lowerBoundToFloatBase(int value) const {
		return static_cast<double>(value * bin_size + 1);
	}
	QString name() const override {
		return StatsTranslations::tr("in %L2 steps").arg(bin_size);
	}
	int to_bin_value(const dive *d) const {
		if (d->number <= 0)
			return invalid_value<int>();
		return (d->number - 1) / bin_size;
	}
};

static DiveNrBinner dive_nr_binner_5(5);
static DiveNrBinner dive_nr_binner_10(10);
static DiveNrBinner dive_nr_binner_20(20);
static DiveNrBinner dive_nr_binner_50(50);
static DiveNrBinner dive_nr_binner_100(100);
static DiveNrBinner dive_nr_binner_200(200);

struct DiveNrVariable : public StatsVariableTemplate<StatsVariable::Type::Numeric> {
	QString name() const override {
		return StatsTranslations::tr("Dive #");
	}
	std::vector<const StatsBinner *> binners() const override {
		if (divelog.dives->nr > 1000)
			return { &dive_nr_binner_20, &dive_nr_binner_50, &dive_nr_binner_100, &dive_nr_binner_200 };
		else
			return { &dive_nr_binner_5, &dive_nr_binner_10, &dive_nr_binner_20, &dive_nr_binner_50 };
	}
	double toFloat(const dive *d) const override {
		return d->number >= 0 ? static_cast<double>(d->number)
				      : invalid_value<double>();
	}
	std::vector<StatsOperation> supportedOperations() const override {
		return { StatsOperation::Median, StatsOperation::Mean };
	}
};

// ============ Dive mode ============

struct DiveModeBinner : public SimpleBinner<DiveModeBinner, IntBin> {
	QString format(const StatsBin &bin) const override {
		return QString(divemode_text_ui[derived_bin(bin).value]);
	}
	int to_bin_value(const dive *d) const {
		int res = (int)d->dc.divemode;
		return res >= 0 && res < NUM_DIVEMODE ? res : OC;
	}
};

static DiveModeBinner dive_mode_binner;
struct DiveModeVariable : public StatsVariableTemplate<StatsVariable::Type::Discrete> {
	QString name() const override {
		return StatsTranslations::tr("Dive mode");
	}
	QString diveCategories(const dive *d) const override {
		int mode = (int)d->dc.divemode;
		return mode >= 0 && mode < NUM_DIVEMODE ?
			QString(divemode_text_ui[mode]) : QString();
	}
	std::vector<const StatsBinner *> binners() const override {
		return { &dive_mode_binner };
	}
};

// ============ People, buddies and dive guides ============

struct PeopleBinner : public StringBinner<PeopleBinner, StringBin> {
	std::vector<QString> to_bin_values(const dive *d) const {
		std::vector<QString> dive_people;
		for (const QString &s: QString(d->buddy).split(",", SKIP_EMPTY))
			dive_people.push_back(s.trimmed());
		for (const QString &s: QString(d->diveguide).split(",", SKIP_EMPTY))
			dive_people.push_back(s.trimmed());
		return dive_people;
	}
};

static PeopleBinner people_binner;
struct PeopleVariable : public StatsVariableTemplate<StatsVariable::Type::Discrete> {
	QString name() const override {
		return StatsTranslations::tr("People");
	}
	QString diveCategories(const dive *d) const override {
		QString buddy = QString(d->buddy).trimmed();
		QString diveguide = QString(d->diveguide).trimmed();
		if (!buddy.isEmpty() && !diveguide.isEmpty())
			buddy += ", ";
		return buddy + diveguide;
	}
	std::vector<const StatsBinner *> binners() const override {
		return { &people_binner };
	}
};

struct BuddyBinner : public StringBinner<BuddyBinner, StringBin> {
	std::vector<QString> to_bin_values(const dive *d) const {
		std::vector<QString> buddies;
		for (const QString &s: QString(d->buddy).split(",", SKIP_EMPTY))
			buddies.push_back(s.trimmed());
		return buddies;
	}
};

static BuddyBinner buddy_binner;
struct BuddyVariable : public StatsVariableTemplate<StatsVariable::Type::Discrete> {
	QString name() const override {
		return StatsTranslations::tr("Buddies");
	}
	QString diveCategories(const dive *d) const override {
		return QString(d->buddy).trimmed();
	}
	std::vector<const StatsBinner *> binners() const override {
		return { &buddy_binner };
	}
};

struct DiveGuideBinner : public StringBinner<DiveGuideBinner, StringBin> {
	std::vector<QString> to_bin_values(const dive *d) const {
		std::vector<QString> dive_guides;
		for (const QString &s: QString(d->diveguide).split(",", SKIP_EMPTY))
			dive_guides.push_back(s.trimmed());
		return dive_guides;
	}
};

static DiveGuideBinner dive_guide_binner;
struct DiveGuideVariable : public StatsVariableTemplate<StatsVariable::Type::Discrete> {
	QString name() const override {
		return StatsTranslations::tr("Dive guides");
	}
	QString diveCategories(const dive *d) const override {
		return QString(d->diveguide).trimmed();
	}
	std::vector<const StatsBinner *> binners() const override {
		return { &dive_guide_binner };
	}
};

// ============ Tags ============

struct TagBinner : public StringBinner<TagBinner, StringBin> {
	std::vector<QString> to_bin_values(const dive *d) const {
		std::vector<QString> tags;
		for (const tag_entry *tag = d->tag_list; tag; tag = tag->next)
			tags.push_back(QString(tag->tag->name).trimmed());
		return tags;
	}
};

static TagBinner tag_binner;
struct TagVariable : public StatsVariableTemplate<StatsVariable::Type::Discrete> {
	QString name() const override {
		return StatsTranslations::tr("Tags");
	}
	QString diveCategories(const dive *d) const override {
		return get_taglist_string(d->tag_list);
	}
	std::vector<const StatsBinner *> binners() const override {
		return { &tag_binner };
	}
};

// ============ Gas type, in 2%, 5%, 10% and 20% steps  ============
// This is a bit convoluted: We differentiate between four types: air, pure oxygen, EAN and trimix
// The latter two are binned in x% steps. The problem is that we can't use the "simple binner",
// because a dive can have more than one cylinder. Moreover, the string-binner might not be optimal
// because we don't want to format a string for every gas types, when there are thousands of dives!
// Note: when the same dive has multiple cylinders that fall inside a bin, that cylinder will
// only be counted once. Thus, depending on the bin size, the number of entries may change!
// In addition to a binner with percent-steps also provide a general-type binner (air, nitrox, etc.)

// bin gasmix with size given in percent
struct gas_bin_t bin_gasmix(struct gasmix mix, int size)
{
	if (gasmix_is_air(mix))
		return gas_bin_t::air();
	if (get_o2(mix) == 1000)
		return gas_bin_t::oxygen();
	return get_he(mix) == 0 ?
		gas_bin_t::ean(get_o2(mix) / 10 / size * size) :
		gas_bin_t::trimix(get_o2(mix) / 10 / size * size,
				  get_he(mix) / 10 / size * size);
}

struct GasTypeBinner : public MultiBinner<GasTypeBinner, GasTypeBin> {
	int bin_size;
	GasTypeBinner(int size)
		: bin_size(size)
	{
	}
	QString name() const override {
		return StatsTranslations::tr("in %1% steps").arg(bin_size);
	}
	std::vector<gas_bin_t> to_bin_values(const dive *d) const {
		std::vector<gas_bin_t> res;
		res.reserve(d->cylinders.nr);
		for (int i = 0; i < d->cylinders.nr; ++i) {
			struct gasmix mix = d->cylinders.cylinders[i].gasmix;
			if (gasmix_is_invalid(mix))
				continue;
			// Add dive to each bin only once.
			add_to_vector_unique(res, bin_gasmix(mix, bin_size));
		}
		return res;
	}
	QString format(const StatsBin &bin) const override {
		gas_bin_t type = derived_bin(bin).value;
		QLocale loc;
		switch (type.type) {
		default:
		case gas_bin_t::Type::Air:
			return StatsTranslations::tr("Air");
		case gas_bin_t::Type::Oxygen:
			return StatsTranslations::tr("Oxygen");
		case gas_bin_t::Type::EAN:
			return StatsTranslations::tr("EAN%1–%2").arg(loc.toString(type.o2),
								     loc.toString(type.o2 + bin_size - 1));
		case gas_bin_t::Type::Trimix:
			return StatsTranslations::tr("%1/%2–%3/%4").arg(loc.toString(type.o2),
									loc.toString(type.he),
									loc.toString(type.o2 + bin_size - 1),
									loc.toString(type.he + bin_size - 1));
		}
	}
};

struct GasTypeGeneralBinner : public MultiBinner<GasTypeGeneralBinner, IntBin> {
	using MultiBinner::MultiBinner;
	QString name() const override {
		return StatsTranslations::tr("General");
	}
	std::vector<int> to_bin_values(const dive *d) const {
		std::vector<int> res;
		res.reserve(d->cylinders.nr);
		for (int i = 0; i < d->cylinders.nr; ++i) {
			struct gasmix mix = d->cylinders.cylinders[i].gasmix;
			if (gasmix_is_invalid(mix))
				continue;
			res.push_back(gasmix_to_type(mix));
		}
		return res;
	}
	QString format(const StatsBin &bin) const override {
		int type = derived_bin(bin).value;
		return gastype_name((gastype)type);
	}
};

static GasTypeGeneralBinner gas_type_general_binner;
static GasTypeBinner gas_type_binner2(2);
static GasTypeBinner gas_type_binner5(5);
static GasTypeBinner gas_type_binner10(10);
static GasTypeBinner gas_type_binner20(20);

struct GasTypeVariable : public StatsVariableTemplate<StatsVariable::Type::Discrete> {
	QString name() const override {
		return StatsTranslations::tr("Gas type");
	}
	QString diveCategories(const dive *d) const override {
		QString res;
		std::vector<gasmix> mixes;	// List multiple cylinders only once
		mixes.reserve(d->cylinders.nr);
		for (int i = 0; i < d->cylinders.nr; ++i) {
			struct gasmix mix = d->cylinders.cylinders[i].gasmix;
			if (gasmix_is_invalid(mix))
				continue;
			if (std::find_if(mixes.begin(), mixes.end(),
					 [mix] (gasmix mix2)
					 { return same_gasmix(mix, mix2); }) != mixes.end())
				continue;
			mixes.push_back(mix);
			if (!res.isEmpty())
				res += ", ";
			res += get_gas_string(mix);
		}
		return res;
	}
	std::vector<const StatsBinner *> binners() const override {
		return { &gas_type_general_binner,
			 &gas_type_binner2, &gas_type_binner5, &gas_type_binner10, &gas_type_binner20 };
	}
};

// ============ O2 and H2 content, binned in 2, 5, 10, 20 % bins ============

// Get the gas content in permille of a dive, based on two flags:
//  - he: get he content, otherwise o2
//  - max_he: get cylinder with maximum he content, otherwise with maximum o2 content
static int get_gas_content(const struct dive *d, bool he, bool max_he)
{
	if (d->cylinders.nr <= 0)
		return invalid_value<int>();
	// If sorting be He, the second sort criterion is O2 descending, because
	// we are interested in the "bottom gas": highest He and lowest O2.
	auto comp = max_he ? [] (const cylinder_t &c1, const cylinder_t &c2)
				{ return std::make_tuple(get_he(c1.gasmix), -get_o2(c1.gasmix)) <
					 std::make_tuple(get_he(c2.gasmix), -get_o2(c2.gasmix)); }
			   : [] (const cylinder_t &c1, const cylinder_t &c2)
				{ return get_o2(c1.gasmix) < get_o2(c2.gasmix); };
	auto it = std::max_element(d->cylinders.cylinders, d->cylinders.cylinders + d->cylinders.nr, comp);
	return he ? get_he(it->gasmix) : get_o2(it->gasmix);
}

// We use the same binner for all gas contents
struct GasContentBinner : public IntRangeBinner<GasContentBinner, IntBin> {
	bool he; // true if this returns He content, otherwise O2
	bool max_he; // true if this takes the gas with maximum helium, otherwise maximum O2
	GasContentBinner(int bin_size, bool he, bool max_he) : IntRangeBinner(bin_size),
		he(he), max_he(max_he)
	{
	}
	QString name() const override {
		return StatsTranslations::tr("In %L1% steps").arg(bin_size);
	}
	QString unitSymbol() const override {
		return "%";
	}
	int to_bin_value(const struct dive *d) const {
		int res = get_gas_content(d, he, max_he);
		// Convert to percent and then bin, but take care not to mangle the invalid value.
		return is_invalid_value(res) ? res : res / 10 / bin_size;
	}
};

struct GasContentVariable : public StatsVariableTemplate<StatsVariable::Type::Numeric> {

	// In the constructor, generate binners with 2, 5, 10 and 20% bins.
	GasContentBinner b1, b2, b3, b4;
	bool he, max_he;
	GasContentVariable(bool he, bool max_he) :
		b1(2, he, max_he), b2(5, he, max_he),
		b3(10, he, max_he), b4(20, he, max_he),
		he(he), max_he(max_he)
	{
	}
	std::vector<const StatsBinner *> binners() const override {
		return { &b1, &b2, &b3, &b4 };
	}
	QString unitSymbol() const override {
		return "%";
	}
	int decimals() const override {
		return 1;
	}
	std::vector<StatsOperation> supportedOperations() const override {
		return { StatsOperation::Median, StatsOperation::Mean, StatsOperation::TimeWeightedMean, StatsOperation::Min, StatsOperation::Max };
	}
	double toFloat(const dive *d) const override {
		int res = get_gas_content(d, he, max_he);
		// Attn: we have to turn invalid-int into invalid-float.
		// Perhaps we should signal invalid with an std::optional kind of object?
		if (is_invalid_value(res))
			return invalid_value<double>();
		return res / 10.0;
	}
};

struct GasContentO2Variable : GasContentVariable {
	GasContentO2Variable() : GasContentVariable(false, false)
	{
	}
	QString name() const override {
		return StatsTranslations::tr("O₂ (max)");
	}
};

struct GasContentO2HeMaxVariable : GasContentVariable {
	GasContentO2HeMaxVariable() : GasContentVariable(false, true)
	{
	}
	QString name() const override {
		return StatsTranslations::tr("O₂ (bottom gas)");
	}
};

struct GasContentHeVariable : GasContentVariable {
	GasContentHeVariable() : GasContentVariable(true, true)
	{
	}
	QString name() const override {
		return StatsTranslations::tr("He (max)");
	}
};

// ============ Suit ============

struct SuitBinner : public StringBinner<SuitBinner, StringBin> {
	std::vector<QString> to_bin_values(const dive *d) const {
		return { QString(d->suit) };
	}
};

static SuitBinner suit_binner;
struct SuitVariable : public StatsVariableTemplate<StatsVariable::Type::Discrete> {
	QString name() const override {
		return StatsTranslations::tr("Suit type");
	}
	QString diveCategories(const dive *d) const override {
		return QString(d->suit);
	}
	std::vector<const StatsBinner *> binners() const override {
		return { &suit_binner };
	}
};

// ============ Weightsystem ============

static std::vector<QString> weightsystems(const dive *d)
{
	std::vector<QString> res;
	res.reserve(d->weightsystems.nr);
	for (int i = 0; i < d->weightsystems.nr; ++i)
		add_to_vector_unique(res, QString(d->weightsystems.weightsystems[i].description).trimmed());
	return res;
}

struct WeightsystemBinner : public StringBinner<WeightsystemBinner, StringBin> {
	std::vector<QString> to_bin_values(const dive *d) const {
		return weightsystems(d);
	}
};

static WeightsystemBinner weightsystem_binner;
struct WeightsystemVariable : public StatsVariableTemplate<StatsVariable::Type::Discrete> {
	QString name() const override {
		return StatsTranslations::tr("Weightsystem");
	}
	QString diveCategories(const dive *d) const override {
		return join_strings(weightsystems(d));
	}
	std::vector<const StatsBinner *> binners() const override {
		return { &weightsystem_binner };
	}
};

// ============ Cylinder types ============

static std::vector<QString> cylinder_types(const dive *d)
{
	std::vector<QString> res;
	res.reserve(d->cylinders.nr);
	for (int i = 0; i < d->cylinders.nr; ++i)
		add_to_vector_unique(res, QString(d->cylinders.cylinders[i].type.description).trimmed());
	return res;
}

struct CylinderTypeBinner : public StringBinner<CylinderTypeBinner, StringBin> {
	std::vector<QString> to_bin_values(const dive *d) const {
		return cylinder_types(d);
	}
};

static CylinderTypeBinner cylinder_type_binner;
struct CylinderTypeVariable : public StatsVariableTemplate<StatsVariable::Type::Discrete> {
	QString name() const override {
		return StatsTranslations::tr("Cylinder type");
	}
	QString diveCategories(const dive *d) const override {
		return join_strings(cylinder_types(d));
	}
	std::vector<const StatsBinner *> binners() const override {
		return { &cylinder_type_binner };
	}
};

// ============ Location (including trip location) ============

using LocationBin = SimpleBin<DiveSiteWrapper>;

struct LocationBinner : public SimpleBinner<LocationBinner, LocationBin> {
	QString format(const StatsBin &bin) const override {
		return derived_bin(bin).value.format();
	}
	const DiveSiteWrapper to_bin_value(const dive *d) const {
		return DiveSiteWrapper(d->dive_site);
	}
};

static LocationBinner location_binner;
struct LocationVariable : public StatsVariableTemplate<StatsVariable::Type::Discrete> {
	QString name() const override {
		return StatsTranslations::tr("Dive site");
	}
	QString diveCategories(const dive *d) const override {
		return DiveSiteWrapper(d->dive_site).format();
	}
	std::vector<const StatsBinner *> binners() const override {
		return { &location_binner };
	}
};

// ============ Dive trip ============

using TripBin = SimpleBin<TripWrapper>;

struct TripBinner : public SimpleBinner<TripBinner, TripBin> {
	QString format(const StatsBin &bin) const override {
		return derived_bin(bin).value.format();
	}
	const TripWrapper to_bin_value(const dive *d) const {
		return TripWrapper(d->divetrip);
	}
};

static TripBinner trip_binner;
struct TripVariable : public StatsVariableTemplate<StatsVariable::Type::Discrete> {
	QString name() const override {
		return StatsTranslations::tr("Dive trip");
	}
	QString diveCategories(const dive *d) const override {
		return formatTripTitle(d->divetrip);
	}
	std::vector<const StatsBinner *> binners() const override {
		return { &trip_binner };
	}
};

// ============ Day of the week ============

struct DayOfWeekBinner : public SimpleBinner<DayOfWeekBinner, IntBin> {
	QString format(const StatsBin &bin) const override {
		return formatDayOfWeek(derived_bin(bin).value);
	}
	int to_bin_value(const dive *d) const {
		return utc_weekday(d->when);
	}
};

static DayOfWeekBinner day_of_week_binner;
struct DayOfWeekVariable : public StatsVariableTemplate<StatsVariable::Type::Discrete> {
	QString name() const override {
		return StatsTranslations::tr("Day of week");
	}
	QString diveCategories(const dive *d) const override {
		return formatDayOfWeek(utc_weekday(d->when));
	}
	std::vector<const StatsBinner *> binners() const override {
		return { &day_of_week_binner };
	}
};

// ============ Rating ============
struct RatingBinner : public SimpleBinner<RatingBinner, IntBin> {
	QString format(const StatsBin &bin) const override {
		return QString("🌟").repeated(derived_bin(bin).value);
	}

	int to_bin_value(const dive *d) const {
		int res = (int)d->rating;
		return res;
	}
};

static RatingBinner rating_binner;
struct RatingVariable : public StatsVariableTemplate<StatsVariable::Type::Discrete> {
	QString name() const override {
		return StatsTranslations::tr("Rating");
	}
	QString diveCategories(const dive *d) const override {
		int rating = (int)d->rating;
		return QString("🌟").repeated(rating);
	}
	std::vector<const StatsBinner *> binners() const override {
		return { &rating_binner };
	}
};

// ============ Visibility ============
struct VisibilityBinner : public SimpleBinner<VisibilityBinner, IntBin> {
	QString format(const StatsBin &bin) const override {
		return QString("🌟").repeated(derived_bin(bin).value);
	}

	int to_bin_value(const dive *d) const {
		int res = (int)d->visibility;
		return res;
	}
};

static VisibilityBinner visibility_binner;
struct VisibilityVariable : public StatsVariableTemplate<StatsVariable::Type::Discrete> {
	QString name() const override {
		return StatsTranslations::tr("Visibility");
	}
	QString diveCategories(const dive *d) const override {
		int viz = (int)d->visibility;
		return QString("🌟").repeated(viz);
	}
	std::vector<const StatsBinner *> binners() const override {
		return { &visibility_binner };
	}
};

static DateVariable date_variable;
static MaxDepthVariable max_depth_variable;
static MeanDepthVariable mean_depth_variable;
static DurationVariable duration_variable;
static SACVariable sac_variable;
static WaterTemperatureVariable water_temperature_variable;
static AirTemperatureVariable air_temperature_variable;
static WeightVariable weight_variable;
static DiveNrVariable dive_nr_variable;
static DiveModeVariable dive_mode_variable;
static PeopleVariable people_variable;
static BuddyVariable buddy_variable;
static DiveGuideVariable dive_guide_variable;
static TagVariable tag_variable;
static GasTypeVariable gas_type_variable;
static GasContentO2Variable gas_content_o2_variable;
static GasContentO2HeMaxVariable gas_content_o2_he_max_variable;
static GasContentHeVariable gas_content_he_variable;
static SuitVariable suit_variable;
static WeightsystemVariable weightsystem_variable;
static CylinderTypeVariable cylinder_type_variable;
static LocationVariable location_variable;
static TripVariable trip_variable;
static DayOfWeekVariable day_of_week_variable;
static RatingVariable rating_variable;
static VisibilityVariable visibility_variable;

const std::vector<const StatsVariable *> stats_variables = {
	&date_variable, &max_depth_variable, &mean_depth_variable, &duration_variable, &sac_variable,
	&water_temperature_variable, &air_temperature_variable, &weight_variable, &dive_nr_variable,
	&gas_content_o2_variable, &gas_content_o2_he_max_variable, &gas_content_he_variable,
	&dive_mode_variable, &people_variable, &buddy_variable, &dive_guide_variable, &tag_variable,
	&gas_type_variable, &suit_variable,
	&weightsystem_variable, &cylinder_type_variable, &location_variable, &trip_variable, &day_of_week_variable,
	&rating_variable, &visibility_variable
};
