// SPDX-License-Identifier: GPL-2.0
#include "statstypes.h"
#include "statstranslations.h"
#include "core/dive.h"
#include "core/divemode.h"
#include "core/divesite.h"
#include "core/gas.h"
#include "core/pref.h"
#include "core/qthelper.h" // for get_depth_unit() et al.
#include "core/subsurface-time.h"
#include <limits>
#include <QLocale>

#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
#define SKIP_EMPTY Qt::SkipEmptyParts
#else
#define SKIP_EMPTY QString::SkipEmptyParts
#endif

static const constexpr double NaN = std::numeric_limits<double>::quiet_NaN();

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

// Note: usually I dislike functions defined inside class/struct
// declarations ("Java style"). However, for brevity this is done
// in this rather template-heavy source file more or less consistently.

// Templates to define invalid values for types and test for said values.
// This is used by the binners: returning such a value means "ignore this dive".
template<typename T> T invalid_value();
template<> int invalid_value<int>()
{
	return std::numeric_limits<int>::max();
}
template<> double invalid_value<double>()
{
	return std::numeric_limits<double>::quiet_NaN();
}
template<> QString invalid_value<QString>()
{
	return QString();
}
template<> StatsQuartiles invalid_value<StatsQuartiles>()
{
	double NaN = std::numeric_limits<double>::quiet_NaN();
	return { NaN, NaN, NaN, NaN, NaN };
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

static bool is_invalid_value(const dive_site *d)
{
	return !d;
}

static bool is_invalid_value(const StatsOperationResults &res)
{
	return !res.isValid();
}

struct gas_bin_t {
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
	return std::isnan(q.min);
}

bool StatsQuartiles::isValid() const
{
	return !is_invalid_value(*this);
}

// Define an ordering for gas types
// invalid < air < ean (including oxygen) < trimix
// The latter two are sorted by (helium, oxygen)
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

StatsType::~StatsType()
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
	return unit.isEmpty() ? name : QStringLiteral("%1 %2").arg(name, unit);
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

// Default implementation for discrete types: there are no bins between discrete bins.
std::vector<StatsBinPtr> StatsBinner::bins_between(const StatsBin &bin1, const StatsBin &bin2) const
{
	return {};
}

QString StatsType::unitSymbol() const
{
	return {};
}

QString StatsType::diveCategories(const dive *) const
{
	return QString();
}

int StatsType::decimals() const
{
	return 0;
}

double StatsType::toFloat(const dive *d) const
{
	return invalid_value<double>();
}

QString StatsType::nameWithUnit() const
{
	QString s = name();
	QString symb = unitSymbol();
	return symb.isEmpty() ? s : QStringLiteral("%1 [%2]").arg(s, symb);
}

QString StatsType::nameWithBinnerUnit(const StatsBinner &binner) const
{
	QString s = name();
	QString symb = binner.unitSymbol();
	return symb.isEmpty() ? s : QStringLiteral("%1 [%2]").arg(s, symb);
}

const StatsBinner *StatsType::getBinner(int idx) const
{
	std::vector<const StatsBinner *> b = binners();
	if (b.empty())
		return nullptr;
	return idx >= 0 && idx < (int)b.size() ? b[idx] : b[0];
}

std::vector<StatsOperation> StatsType::supportedOperations() const
{
	return {};
}

// Attn: The order must correspond to the StatsOperation enum
static const char *operation_names[] = {
	QT_TRANSLATE_NOOP("StatsTranslations", "Median"),
	QT_TRANSLATE_NOOP("StatsTranslations", "Mean"),
	QT_TRANSLATE_NOOP("StatsTranslations", "Time-weighted mean"),
	QT_TRANSLATE_NOOP("StatsTranslations", "Sum")
};

QStringList StatsType::supportedOperationNames() const
{
	std::vector<StatsOperation> ops = supportedOperations();
	QStringList res;
	res.reserve(ops.size());
	for (StatsOperation op: ops)
		res.push_back(operationName(op));
	return res;
}

StatsOperation StatsType::idxToOperation(int idx) const
{
	std::vector<StatsOperation> ops = supportedOperations();
	if (ops.empty()) {
		qWarning("Stats type %s does not support operations", qPrintable(name()));
		return StatsOperation::Median; // oops!
	}
	return idx < 0 || idx >= (int)ops.size() ? ops[0] : ops[idx];
}

QString StatsType::operationName(StatsOperation op)
{
	int idx = (int)op;
	return idx < 0 || idx >= (int)std::size(operation_names) ? QString()
								 : operation_names[(int)op];
}

double StatsType::mean(const std::vector<dive *> &dives) const
{
	StatsOperationResults res = applyOperations(dives);
	return res.isValid() ? res.mean : invalid_value<double>();
}

std::vector<StatsValue> StatsType::values(const std::vector<dive *> &dives) const
{
	std::vector<StatsValue> vec;
	vec.reserve(dives.size());
	for (dive *d: dives) {
		double v = toFloat(d);
		if (is_invalid_value(v))
			continue;
		vec.push_back({ v, d });
	}
	std::sort(vec.begin(), vec.end(),
		  [](const StatsValue &v1, const StatsValue &v2)
		  { return v1.v < v2.v; });
	return vec;
}

QString StatsType::valueWithUnit(const dive *d) const
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

StatsQuartiles StatsType::quartiles(const std::vector<dive *> &dives) const
{
	return quartiles(values(dives));
}

// This expects the value vector to be sorted!
StatsQuartiles StatsType::quartiles(const std::vector<StatsValue> &vec)
{
	size_t s = vec.size();
	if (s == 0)
		return invalid_value<StatsQuartiles>();
	switch (s % 4) {
	default:
		// gcc doesn't recognize that we catch all possible values. disappointing.
	case 0:
		return { vec[0].v, q3(&vec[s/4 - 1]), q2(&vec[s/2 - 1]), q1(&vec[s - s/4 - 1]), vec[s - 1].v };
	case 1:
		return { vec[0].v, vec[s/4].v, vec[s/2].v, vec[s - s/4 - 1].v, vec[s - 1].v };
	case 2:
		return { vec[0].v, q1(&vec[s/4]), q2(&vec[s/2 - 1]), q3(&vec[s - s/4 - 2]), vec[s - 1].v };
	case 3:
		return { vec[0].v, q2(&vec[s/4]), vec[s/2].v, q2(&vec[s - s/4 - 2]), vec[s - 1].v };
	}
}

StatsOperationResults StatsType::applyOperations(const std::vector<dive *> &dives) const
{
	StatsOperationResults res;
	std::vector<StatsValue> val = values(dives);

	double sumTime = 0.0;
	res.count = (int)val.size();
	res.median = quartiles(val).q2;

	if (res.count <= 0)
		return res;

	for (auto [v, d]: val) {
		res.sum += v;
		res.mean += v;
		sumTime += d->duration.seconds;
		res.timeWeightedMean += v * d->duration.seconds;
	}

	res.mean /= res.count;
	res.timeWeightedMean /= sumTime;
	return res;
}

StatsOperationResults::StatsOperationResults() :
	count(0), median(0.0), mean(0.0), timeWeightedMean(0.0), sum(0.0)
{
}

bool StatsOperationResults::isValid() const
{
	return count > 0;
}

double StatsOperationResults::get(StatsOperation op) const
{
	switch (op) {
	case StatsOperation::Median: return median;
	case StatsOperation::Mean: return mean;
	case StatsOperation::TimeWeightedMean: return timeWeightedMean;
	case StatsOperation::Sum: return sum;
	case StatsOperation::Invalid:
	default: return invalid_value<double>();
	}
}

std::vector<StatsScatterItem> StatsType::scatter(const StatsType &t2, const std::vector<dive *> &dives) const
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
std::vector<StatsBinValue<T>> bin_convert(const StatsType &type, const StatsBinner &binner, const std::vector<dive *> &dives,
					  bool fill_empty, DivesToValueFunc func)
{
	std::vector<StatsBinDives> bin_dives = binner.bin_dives(dives, fill_empty);
	std::vector<StatsBinValue<T>> res;
	res.reserve(bin_dives.size());
	for (auto &[bin, dives]: bin_dives) {
		T v = func(dives);
		if (is_invalid_value(v) && (res.empty() || !fill_empty))
			continue;
		res.push_back({ std::move(bin), v });
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

std::vector<StatsBinQuartiles> StatsType::bin_quartiles(const StatsBinner &binner, const std::vector<dive *> &dives, bool fill_empty) const
{
	return bin_convert<StatsQuartiles>(*this, binner, dives, fill_empty,
					   [this](const std::vector<dive *> &d) { return quartiles(d); });
}

std::vector<StatsBinOp> StatsType::bin_operations(const StatsBinner &binner, const std::vector<dive *> &dives, bool fill_empty) const
{
	return bin_convert<StatsOperationResults>(*this, binner, dives, fill_empty,
						  [this](const std::vector<dive *> &d) { return applyOperations(d); });
}

std::vector<StatsBinValues> StatsType::bin_values(const StatsBinner &binner, const std::vector<dive *> &dives, bool fill_empty) const
{
	return bin_convert<std::vector<StatsValue>>(*this, binner, dives, fill_empty,
						    [this](const std::vector<dive *> &d) { return values(d); });
}

// Silly template, which spares us defining type() member functions.
template<StatsType::Type t>
struct StatsTypeTemplate : public StatsType {
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
//    into a double which is understood by the StatsType.
// The bins must possess:
//  - A member variable "value" of the type it is constructed with.
// Note: this uses the curiously recurring template pattern, which I
// dislike, but it is the easiest thing for now.
template<typename Binner, typename Bin>
struct SimpleBinner : public StatsBinner {
public:
	using Type = decltype(Bin::value);
	std::vector<StatsBinDives> bin_dives(const std::vector<dive *> &dives, bool fill_empty) const override;
	std::vector<StatsBinCount> count_dives(const std::vector<dive *> &dives, bool fill_empty) const override;
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
	// out of that. I wonder if that is permature optimization?
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

template<typename Binner, typename Bin>
std::vector<StatsBinCount> SimpleBinner<Binner, Bin>::count_dives(const std::vector<dive *> &dives, bool fill_empty) const
{
	// First, collect a value / counts vector and then produce the final vector
	// out of that. I wonder if that is permature optimization?
	using Pair = std::pair<Type, int>;
	std::vector<Pair> value_bins;
	for (const dive *d: dives) {
		Type value = derived().to_bin_value(d);
		if (is_invalid_value(value))
			continue;
		register_bin_value(value_bins, value, [](int &i){ ++i; });
	}

	// Now, turn that into our result array with allocated bin objects.
	return value_vector_to_bin_vector<Bin>(*this, value_bins, fill_empty);
}

// A simple binner (see above) that works on continuous (or numeric) types
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

// A binner template for discrete types that where each dive can belong to
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
	std::vector<StatsBinCount> count_dives(const std::vector<dive *> &dives, bool fill_empty) const override;
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
	// out of that. I wonder if that is permature optimization?
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

template<typename Binner, typename Bin>
std::vector<StatsBinCount> MultiBinner<Binner, Bin>::count_dives(const std::vector<dive *> &dives, bool) const
{
	// First, collect a value / counts vector and then produce the final vector
	// out of that. I wonder if that is permature optimization?
	using Pair = std::pair<Type, int>;
	std::vector<Pair> value_bins;
	for (const dive *d: dives) {
		for (const Type &s: derived().to_bin_values(d)) {
			if (is_invalid_value(s))
				continue;
			register_bin_value(value_bins, s, [](int &i){ ++i; });
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
	// In histograms, output year for fill years, month otherwise
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
struct DateType : public StatsTypeTemplate<StatsType::Type::Continuous> {
	QString name() const {
		return StatsTranslations::tr("Date");
	}
	std::vector<const StatsBinner *> binners() const override {
		return { &date_year_binner, &date_quarter_binner, &date_month_binner };
	}
};

// ============ Dive depth, binned in 5, 10, 20 m or 15, 30, 60 ft bins ============

struct DepthBinner : public IntRangeBinner<DepthBinner, IntBin> {
	bool metric;
	DepthBinner(int bin_size, bool metric) : IntRangeBinner(bin_size), metric(metric)
	{
	}
	QString name() const override {
		QLocale loc;
		return StatsTranslations::tr("in %1 %2 steps").arg(loc.toString(bin_size),
								   get_depth_unit());
	}
	QString unitSymbol() const override {
		return get_depth_unit();
	}
	int to_bin_value(const dive *d) const {
		return metric ? d->maxdepth.mm / 1000 / bin_size
			      : lrint(mm_to_feet(d->maxdepth.mm)) / bin_size;
	}
};

static DepthBinner meter_binner5(5, true);
static DepthBinner meter_binner10(10, true);
static DepthBinner meter_binner20(20, true);
static DepthBinner feet_binner15(15, false);
static DepthBinner feet_binner30(30, false);
static DepthBinner feet_binner60(60, false);
struct DepthType : public StatsTypeTemplate<StatsType::Type::Numeric> {
	QString name() const override {
		return StatsTranslations::tr("Max. Depth");
	}
	QString unitSymbol() const override {
		return get_depth_unit();
	}
	int decimals() const override {
		return 1;
	}
	std::vector<const StatsBinner *> binners() const override {
		if (prefs.units.length == units::METERS)
			return { &meter_binner5, &meter_binner10, &meter_binner20 };
		else
			return { &feet_binner15, &feet_binner30, &feet_binner60 };
	}
	double toFloat(const dive *d) const override {
		return prefs.units.length == units::METERS ? d->maxdepth.mm / 1000.0
							   : mm_to_feet(d->maxdepth.mm);
	}
	std::vector<StatsOperation> supportedOperations() const override {
		return { StatsOperation::Median, StatsOperation::Mean, StatsOperation::Sum };
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
struct DurationType : public StatsTypeTemplate<StatsType::Type::Numeric> {
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
		return { StatsOperation::Median, StatsOperation::Mean, StatsOperation::Sum };
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
		return get_volume_unit() + StatsTranslations::tr("/min");
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
		return get_volume_unit() + StatsTranslations::tr("/min");
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

struct SACType : public StatsTypeTemplate<StatsType::Type::Numeric> {
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
		return { StatsOperation::Median, StatsOperation::Mean, StatsOperation::TimeWeightedMean };
	}
};

// ============ Water and air temperature, binned in 2, 5, 10, 20 °C/°F bins ============

// We use the same binner for °C and °F. We simply output the current temperature unit.
struct TemperatureBinner : public IntRangeBinner<TemperatureBinner, IntBin> {
	bool air;
	TemperatureBinner(int bin_size, bool air) :
		IntRangeBinner(bin_size),
		air(air)
	{
	}
	QString name() const override {
		QLocale loc;
		return StatsTranslations::tr("in %1 %2 steps").arg(loc.toString(bin_size),
								   get_temp_unit());
	}
	QString unitSymbol() const override {
		return get_temp_unit();
	}
	int to_bin_value(const struct dive *d) const {
		temperature_t t = air ? d->airtemp : d->watertemp;
		if (t.mkelvin <= 0)
			return invalid_value<int>();
		int temp = (int)floor(prefs.units.temperature == units::CELSIUS ?
				mkelvin_to_C(t.mkelvin) : mkelvin_to_F(t.mkelvin));
		return temp / bin_size;
	}
};

TemperatureBinner water_temperature_binner2(2, false);
TemperatureBinner water_temperature_binner5(5, false);
TemperatureBinner water_temperature_binner10(10, false);
TemperatureBinner water_temperature_binner20(20, false);
TemperatureBinner air_temperature_binner2(2, true);
TemperatureBinner air_temperature_binner5(5, true);
TemperatureBinner air_temperature_binner10(10, true);
TemperatureBinner air_temperature_binner20(20, true);

struct TemperatureType : public StatsTypeTemplate<StatsType::Type::Numeric> {
	// Generate 2, 5, 10 and 20-units binners in the constructor
	TemperatureBinner b2, b5, b10, b20;
	TemperatureType(bool air) : b2(2, air), b5(5, air), b10(10, air), b20(20, air)
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
		return { &b2, &b5, &b10, &b20 };
	}
	std::vector<StatsOperation> supportedOperations() const override {
		return { StatsOperation::Median, StatsOperation::Mean, StatsOperation::TimeWeightedMean };
	}
};

struct WaterTemperatureType : TemperatureType {
	WaterTemperatureType() : TemperatureType(false)
	{
	}
	QString name() const override {
		return StatsTranslations::tr("Water temperature");
	}
	double toFloat(const dive *d) const override {
		return tempToFloat(d->watertemp);
	}
};

struct AirTemperatureType : TemperatureType {
	AirTemperatureType() : TemperatureType(true)
	{
	}
	QString name() const override {
		return StatsTranslations::tr("Air temperature");
	}
	double toFloat(const dive *d) const override {
		return tempToFloat(d->airtemp);
	}
};

// ============ Weight, binned in 1, 2, 5, 10 m or 2, 4, 10 or 20 ft bins ============

struct WeightBinner : public IntRangeBinner<WeightBinner, IntBin> {
	bool metric;
	WeightBinner(int bin_size, bool metric) : IntRangeBinner(bin_size), metric(metric)
	{
	}
	QString name() const override {
		QLocale loc;
		return StatsTranslations::tr("in %1 %2 steps").arg(loc.toString(bin_size),
								   get_weight_unit());
	}
	QString unitSymbol() const override {
		return get_weight_unit();
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

struct WeightType : public StatsTypeTemplate<StatsType::Type::Numeric> {
	QString name() const override {
		return StatsTranslations::tr("Weight");
	}
	QString unitSymbol() const override {
		return get_weight_unit();
	}
	int decimals() const override {
		return prefs.units.weight == units::KG ? 1 : 0;
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
		return { StatsOperation::Median, StatsOperation::Mean, StatsOperation::Sum };
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
struct DiveModeType : public StatsTypeTemplate<StatsType::Type::Discrete> {
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

// ============ Buddy (including dive guides) ============

struct BuddyBinner : public StringBinner<BuddyBinner, StringBin> {
	std::vector<QString> to_bin_values(const dive *d) const {
		std::vector<QString> dive_people;
		for (const QString &s: QString(d->buddy).split(",", SKIP_EMPTY))
			dive_people.push_back(s.trimmed());
		for (const QString &s: QString(d->divemaster).split(",", SKIP_EMPTY))
			dive_people.push_back(s.trimmed());
		return dive_people;
	}
};

static BuddyBinner buddy_binner;
struct BuddyType : public StatsTypeTemplate<StatsType::Type::Discrete> {
	QString name() const override {
		return StatsTranslations::tr("Buddies");
	}
	QString diveCategories(const dive *d) const override {
		QString buddy = QString(d->buddy).trimmed();
		QString divemaster = QString(d->divemaster).trimmed();
		if (!buddy.isEmpty() && !divemaster.isEmpty())
			buddy += ", ";
		return buddy + divemaster;
	}
	std::vector<const StatsBinner *> binners() const override {
		return { &buddy_binner };
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
	if (mix.o2.permille == 1000)
		return gas_bin_t::oxygen();
	return mix.he.permille == 0 ?
		gas_bin_t::ean(mix.o2.permille / 10 / size * size) :
		gas_bin_t::trimix(mix.o2.permille / 10 / size * size,
				 mix.he.permille / 10 / size * size);
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

struct GasTypeType : public StatsTypeTemplate<StatsType::Type::Discrete> {
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

struct GasContentType : public StatsTypeTemplate<StatsType::Type::Numeric> {

	// In the constructor, generate binners with 2, 5, 10 and 20% bins.
	GasContentBinner b1, b2, b3, b4;
	bool he, max_he;
	GasContentType(bool he, bool max_he) :
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
		return { StatsOperation::Median, StatsOperation::Mean, StatsOperation::TimeWeightedMean };
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

struct GasContentO2Type : GasContentType {
	GasContentO2Type() : GasContentType(false, false)
	{
	}
	QString name() const override {
		return StatsTranslations::tr("O₂ (max)");
	}
};

struct GasContentO2HeMaxType : GasContentType {
	GasContentO2HeMaxType() : GasContentType(false, true)
	{
	}
	QString name() const override {
		return StatsTranslations::tr("O₂ (bottom gas)");
	}
};

struct GasContentHeType : GasContentType {
	GasContentHeType() : GasContentType(true, true)
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
struct SuitType : public StatsTypeTemplate<StatsType::Type::Discrete> {
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
struct WeightsystemType : public StatsTypeTemplate<StatsType::Type::Discrete> {
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
struct CylinderTypeType : public StatsTypeTemplate<StatsType::Type::Discrete> {
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

using LocationBin = SimpleBin<const dive_site *>;

struct LocationBinner : public SimpleBinner<LocationBinner, LocationBin> {
	QString format(const StatsBin &bin) const override {
		const dive_site *ds = derived_bin(bin).value;
		return QString(ds ? ds->name : "-");
	}
	const dive_site *to_bin_value(const dive *d) const {
		return d->dive_site;
	}
};

static LocationBinner location_binner;
struct LocationType : public StatsTypeTemplate<StatsType::Type::Discrete> {
	QString name() const override {
		return StatsTranslations::tr("Dive site");
	}
	QString diveCategories(const dive *d) const override {
		return d->dive_site ? d->dive_site->name : "-";
	}
	std::vector<const StatsBinner *> binners() const override {
		return { &location_binner };
	}
};

static DateType date_type;
static DepthType depth_type;
static DurationType duration_type;
static SACType sac_type;
static WaterTemperatureType water_temperature_type;
static AirTemperatureType air_temperature_type;
static WeightType weight_type;
static DiveModeType dive_mode_type;
static BuddyType buddy_type;
static GasTypeType gas_type_type;
static GasContentO2Type gas_content_o2_type;
static GasContentO2HeMaxType gas_content_o2_he_max_type;
static GasContentHeType gas_content_he_type;
static SuitType suit_type;
static WeightsystemType weightsystem_type;
static CylinderTypeType cylinder_type_type;
static LocationType location_type;
const std::vector<const StatsType *> stats_types = {
	&date_type, &depth_type, &duration_type, &sac_type,
	&water_temperature_type, &air_temperature_type, &weight_type,
	&gas_content_o2_type, &gas_content_o2_he_max_type, &gas_content_he_type,
	&dive_mode_type, &buddy_type, &gas_type_type, &suit_type,
	&weightsystem_type, &cylinder_type_type, &location_type
};
