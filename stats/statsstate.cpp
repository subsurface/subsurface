// SPDX-License-Identifier: GPL-2.0
#include "statsstate.h"
#include "statstypes.h"
#include "statstranslations.h"

// Attn: The order must correspond to the enum above
static const char *chart_subtype_names[] = {
	QT_TRANSLATE_NOOP("StatsTranslations", "Vertical"),
	QT_TRANSLATE_NOOP("StatsTranslations", "Vertical stacked"),
	QT_TRANSLATE_NOOP("StatsTranslations", "Horizontal"),
	QT_TRANSLATE_NOOP("StatsTranslations", "Horizontal stacked"),
	QT_TRANSLATE_NOOP("StatsTranslations", "Pie")
};

enum class SupportedVariable {
	Count,
	Categorical,		// Implies that the variable is binned
	Continuous,		// Implies that the variable is binned
	Numeric
};

static const int ChartFeatureLabels = 1 << 0;
static const int ChartFeatureMedian = 1 << 1;
static const int ChartFeatureMean =   1 << 2;

static const struct ChartTypeDesc {
	ChartType id;
	const char *name;
	SupportedVariable var1;
	SupportedVariable var2;
	bool var2HasOperations;
	const std::vector<ChartSubType> subtypes;
	int features;
} chart_types[] = {
	{
		ChartType::ScatterPlot,
		QT_TRANSLATE_NOOP("StatsTranslations", "Scatter"),
		SupportedVariable::Numeric,
		SupportedVariable::Numeric,
		false,
		{ },
		0
	},
	{
		ChartType::HistogramCount,
		QT_TRANSLATE_NOOP("StatsTranslations", "Histogram"),
		SupportedVariable::Continuous,
		SupportedVariable::Count,
		false,
		{ ChartSubType::Vertical, ChartSubType::Horizontal },
		ChartFeatureLabels | ChartFeatureMedian | ChartFeatureMean
	},
	{
		ChartType::HistogramBar,
		QT_TRANSLATE_NOOP("StatsTranslations", "Histogram"),
		SupportedVariable::Continuous,
		SupportedVariable::Numeric,
		true,
		{ ChartSubType::Vertical, ChartSubType::Horizontal },
		ChartFeatureLabels
	},
	{
		ChartType::DiscreteScatter,
		QT_TRANSLATE_NOOP("StatsTranslations", "Categorical scatter"),
		SupportedVariable::Categorical,
		SupportedVariable::Numeric,
		false,
		{ },
		0
	},
	{
		ChartType::DiscreteBar,
		QT_TRANSLATE_NOOP("StatsTranslations", "Bar"),
		SupportedVariable::Categorical,
		SupportedVariable::Categorical,
		false,
		{ ChartSubType::Vertical, ChartSubType::VerticalStacked, ChartSubType::Horizontal, ChartSubType::HorizontalStacked },
		0
	},
	{
		ChartType::DiscreteValue,
		QT_TRANSLATE_NOOP("StatsTranslations", "Categorical"),
		SupportedVariable::Categorical,
		SupportedVariable::Numeric,
		true,
		{ ChartSubType::Vertical, ChartSubType::Horizontal },
		ChartFeatureLabels
	},
	{
		ChartType::DiscreteCount,
		QT_TRANSLATE_NOOP("StatsTranslations", "Categorical"),
		SupportedVariable::Categorical,
		SupportedVariable::Count,
		false,
		{ ChartSubType::Pie, ChartSubType::Vertical, ChartSubType::Horizontal },
		ChartFeatureLabels
	},
	{
		ChartType::DiscreteBox,
		QT_TRANSLATE_NOOP("StatsTranslations", "Categorical box"),
		SupportedVariable::Categorical,
		SupportedVariable::Numeric,
		false,
		{ },
		0
	}
};

// Some charts are valid, but not preferrable. For example a numeric variable
// is better plotted in a histogram than in a categorical bar chart. To
// describe this use an enum: good, bad, invalid. Default to "good" charts
// first, but ultimately let the user decide.
enum ChartValidity {
	Good,
	Undesired,
	Invalid
};

static const int count_idx = -1; // Special index for the count variable

StatsState::StatsState() :
	var1(stats_types[0]),
	var2(nullptr),
	type(ChartType::DiscreteBar),
	subtype(ChartSubType::Pie),
	labels(true),
	median(false),
	mean(false),
	var1Binner(nullptr),
	var2Binner(nullptr),
	var2Operation(StatsOperation::Invalid),
	var1Binned(false),
	var2Binned(false),
	var2HasOperations(false)
{
	validate(true);
}

static StatsState::VariableList createVariableList(const StatsType *selected, bool addCount, const StatsType *omit)
{
	StatsState::VariableList res;
	res.variables.reserve(stats_types.size() + addCount);
	res.selected = -1;
	if (addCount) {
		if (selected == nullptr)
			res.selected = (int)res.variables.size();
		res.variables.push_back({ StatsTranslations::tr("Count"), count_idx });
	}
	for (int i = 0; i < (int)stats_types.size(); ++i) {
		const StatsType *variable = stats_types[i];
		if (variable == omit)
			continue;
		if (variable == selected)
			res.selected = (int)res.variables.size();
		res.variables.push_back({ variable->name(), i });
	}
	return res;
}

// This is a bit lame: we pass Chart/SubChart as an integer to the UI,
// by placing one in the lower and one in the upper 16 bit of a 32 bit integer.
static int toInt(ChartType type)
{
	return (int)type << 16;
}

static int toInt(ChartType type, ChartSubType subtype)
{
	return ((int)type << 16) | (int)subtype;
}

static std::pair<ChartType, ChartSubType> fromInt(int id)
{
	return { (ChartType)(id >> 16), (ChartSubType)(id & 0xff) };
}

static ChartValidity variableValidity(StatsType::Type type, SupportedVariable var)
{
	switch (var) {
	default:
	case SupportedVariable::Count:
		return ChartValidity::Invalid;	// Count has been special cased outside of this function
	case SupportedVariable::Categorical:
		return type == StatsType::Type::Continuous || type == StatsType::Type::Numeric ?
			ChartValidity::Undesired : ChartValidity::Good;
	case SupportedVariable::Continuous:
		return type == StatsType::Type::Discrete ? ChartValidity::Invalid : ChartValidity::Good;
	case SupportedVariable::Numeric:
		return type != StatsType::Type::Numeric ? ChartValidity::Invalid : ChartValidity::Good;
	}
}

static ChartValidity chartValidity(const ChartTypeDesc &desc, const StatsType *var1, const StatsType *var2)
{
	if (!var1)
		return ChartValidity::Invalid; // Huh? We don't support count as independent variable

	// Check the first variable
	ChartValidity valid1 = variableValidity(var1->type(), desc.var1);
	if (valid1 == ChartValidity::Invalid)
		return ChartValidity::Invalid;

	// Then, check the second variable
	if (var2 == nullptr) // Our special marker for "count"
		return desc.var2 == SupportedVariable::Count ? valid1 : ChartValidity::Invalid;

	ChartValidity valid2 = variableValidity(var2->type(), desc.var2);
	if (valid2 == ChartValidity::Invalid)
		return ChartValidity::Invalid;

	return valid1 == ChartValidity::Undesired || valid2 == ChartValidity::Undesired ? 
		ChartValidity::Undesired : ChartValidity::Good;
}

// Returns a list of (chart-type, warning) pairs
const std::vector<std::pair<const ChartTypeDesc &, bool>> validCharts(const StatsType *var1, const StatsType *var2)
{
	std::vector<std::pair<const ChartTypeDesc &, bool>> res;
	res.reserve(std::size(chart_types));
	for (const ChartTypeDesc &desc: chart_types) {
		ChartValidity valid = chartValidity(desc, var1, var2);
		if (valid == ChartValidity::Invalid)
			continue;
		res.emplace_back(desc, valid == ChartValidity::Undesired);
	}

	return res;
}

static StatsState::ChartList createChartList(const StatsType *var1, const StatsType *var2, ChartType selectedType, ChartSubType selectedSubType)
{
	StatsState::ChartList res;
	res.selected = -1;
	for (auto [desc, warn]: validCharts(var1, var2)) {
		QString name = StatsTranslations::tr(desc.name);
		if (desc.subtypes.empty()) {
			if (selectedType == desc.id)
				res.selected = (int)res.charts.size();
			res.charts.push_back({ name, toInt(desc.id), warn });
		} else {
			for (ChartSubType subtype: desc.subtypes) {
				if (selectedType == desc.id && selectedSubType == subtype)
					res.selected = (int)res.charts.size();
				QString fullname = QString("%1 %2").arg(name,
									StatsTranslations::tr(chart_subtype_names[(int)subtype]));
				res.charts.push_back({ fullname , toInt(desc.id, subtype), warn });
			}
		}
	}

	// If none of the charts are recommended - remove the warning flag.
	// This can happen if if first variable is numerical, but the second is categorical.
	if (std::all_of(res.charts.begin(), res.charts.end(), [] (const StatsState::Chart &c) { return c.warning; })) {
		for (StatsState::Chart &c: res.charts)
			c.warning = false;
	}

	return res;
}

static StatsState::BinnerList createBinnerList(bool binned, const StatsType *var, const StatsBinner *binner)
{
	StatsState::BinnerList res;
	res.selected = -1;
	if (!binned || !var)
		return res;
	std::vector<const StatsBinner *> binners = var->binners();
	if (binners.size() <= 1)
		return res; // Don't show combo boxes for single binners
	res.binners.reserve(binners.size());
	for (const StatsBinner *bin: binners) {
		if (bin == binner)
			res.selected = (int)res.binners.size();
		res.binners.push_back(bin->name());
	}
	return res;
}

static StatsState::VariableList createOperationsList(bool hasOperations, const StatsType *var, StatsOperation operation)
{
	StatsState::VariableList res;
	res.selected = -1;
	if (!hasOperations || !var)
		return res;
	std::vector<StatsOperation> operations = var->supportedOperations();
	res.variables.reserve(operations.size());
	for (StatsOperation op: operations) {
		if (op == operation)
			res.selected = (int)res.variables.size();
		res.variables.push_back({ StatsType::operationName(op), (int)op });
	}
	return res;
}

std::vector<StatsState::Feature> createFeaturesList(int chartFeatures, bool labels, bool median, bool mean)
{
	std::vector<StatsState::Feature> res;
	if (chartFeatures & ChartFeatureLabels)
		res.push_back({ StatsTranslations::tr("labels"), ChartFeatureLabels, labels });
	if (chartFeatures & ChartFeatureMedian)
		res.push_back({ StatsTranslations::tr("median"), ChartFeatureMedian, median });
	if (chartFeatures & ChartFeatureMean)
		res.push_back({ StatsTranslations::tr("mean"), ChartFeatureMean, mean });
	return res;
}

StatsState::UIState StatsState::getUIState() const
{
	UIState res;
	res.var1 = createVariableList(var1, false, nullptr);
	res.var2 = createVariableList(var2, true, var1);
	res.var1Name = var1 ? var1->name() : QString();
	res.var2Name = var2 ? var2->name() : QString();
	res.charts = createChartList(var1, var2, type, subtype);
	res.binners1 = createBinnerList(var1Binned, var1, var1Binner);
	res.binners2 = createBinnerList(var2Binned, var2, var2Binner);
	res.operations2 = createOperationsList(var2HasOperations, var2, var2Operation);
	res.features = createFeaturesList(chartFeatures, labels, median, mean);
	return res;
}

static const StatsBinner *idxToBinner(const StatsType *t, int idx)
{
	if (!t)
		return nullptr;
	auto binners = t->binners();
	return idx >= 0 && idx < (int)binners.size() ? binners[idx] : 0;
}

void StatsState::var1Changed(int id)
{
	var1 = stats_types[std::clamp(id, 0, (int)stats_types.size())];
	validate(true);
}

void StatsState::binner1Changed(int idx)
{
	var1Binner = idxToBinner(var1, idx);
	validate(false);
}

void StatsState::var2Changed(int id)
{
	// The "count" variable is represented by a nullptr
	var2 = id == count_idx ? nullptr
			       : stats_types[std::clamp(id, 0, (int)stats_types.size())];
	validate(true);
}

void StatsState::binner2Changed(int idx)
{
	var2Binner = idxToBinner(var2, idx);
	validate(false);
}

void StatsState::var2OperationChanged(int id)
{
	var2Operation = (StatsOperation)id;
	validate(false);
}

void StatsState::chartChanged(int id)
{
	std::tie(type, subtype) = fromInt(id); // use std::tie to assign two values at once
	validate(false);
}

void StatsState::featureChanged(int id, bool state)
{
	if (id == ChartFeatureLabels)
		labels = state;
	else if (id == ChartFeatureMedian)
		median = state;
	else if (id == ChartFeatureMean)
		mean = state;
}

// Creates the new chart-type from the current chart-type and a list of possible chart types.
// If the flag "varChanged" is true, the current chart-type will be changed if the
// current chart-type is undesired.
const ChartTypeDesc &newChartType(ChartType type, std::vector<std::pair<const ChartTypeDesc &, bool>> charts,
			      bool varChanged)
{
	for (auto [desc, warn]: charts) {
		// Found it, but if the axis was changed, we change anyway if the chart is "undesired"
		if (type == desc.id) {
			if (!varChanged || !warn)
				return desc;
			break;
		}
	}

	// Find the first non-undesired chart
	for (auto [desc, warn]: charts) {
		if (!warn)
			return desc;
	}

	return charts.empty() ? chart_types[0] : charts[0].first;
}

static void validateBinner(const StatsBinner *&binner, const StatsType *var, bool isBinned)
{
	if (!var || !isBinned) {
		binner = nullptr;
		return;
	}
	auto binners = var->binners();
	if (std::find(binners.begin(), binners.end(), binner) != binners.end())
		return;

	// For now choose the first binner. However, we might try to be smarter here
	// and adapt to the given screen size and the estimated number of bins.
	binner = binners.empty() ? nullptr : binners[0];
}

static void validateOperation(StatsOperation &operation, const StatsType *var, bool hasOperation)
{
	if (!hasOperation) {
		operation = StatsOperation::Invalid;
		return;
	}
	std::vector<StatsOperation> ops = var->supportedOperations();
	if (std::find(ops.begin(), ops.end(), operation) != ops.end())
		return;

	operation = ops.empty() ? StatsOperation::Invalid : ops[0];
}

// The var changed variable indicates whether this function is called
// after a variable change or a change of the chart type. In the
// former case, the chart type is switched, if it is not recommended.
// In the latter case, the user explicitly chose a non-recommended type,
// so let's use that.
void StatsState::validate(bool varChanged)
{
	// Take care that we don't plot a variable against itself.
	// By default plot the count of the first variable. Is that sensible?
	if (var1 == var2)
		var2 = nullptr;

	// Let's see if the currently selected chart is one of the valid charts
	auto charts = validCharts(var1, var2);
	const ChartTypeDesc &desc = newChartType(type, charts, varChanged);
	type = desc.id;

	// Check if the current subtype is supported by the chart
	if (std::find(desc.subtypes.begin(), desc.subtypes.end(), subtype) == desc.subtypes.end())
		subtype = desc.subtypes.empty() ? ChartSubType::Pie : desc.subtypes[0];

	var1Binned = desc.var1 == SupportedVariable::Categorical || desc.var1 == SupportedVariable::Continuous;
	var2Binned = desc.var2 == SupportedVariable::Categorical || desc.var2 == SupportedVariable::Continuous;
	var2HasOperations = desc.var2HasOperations;

	chartFeatures = desc.features;
	// Median and mean currently only if first variable is numeric
	if (!var1 || var1->type() != StatsType::Type::Numeric)
		chartFeatures &= ~(ChartFeatureMedian | ChartFeatureMean);

	// Check that the binners and operation are valid
	validateBinner(var1Binner, var1, var1Binned);
	validateBinner(var2Binner, var2, var2Binned);
	validateOperation(var2Operation, var2, var2HasOperations);
}
