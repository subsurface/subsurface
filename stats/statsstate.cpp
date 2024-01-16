// SPDX-License-Identifier: GPL-2.0
#include "statsstate.h"
#include "statstranslations.h"
#include "statsvariables.h"

// Attn: The order must correspond to the enum ChartSubType
static const char *chart_subtype_names[] = {
	QT_TRANSLATE_NOOP("StatsTranslations", "vertical"),
	QT_TRANSLATE_NOOP("StatsTranslations", "grouped vertical"),
	QT_TRANSLATE_NOOP("StatsTranslations", "stacked vertical"),
	QT_TRANSLATE_NOOP("StatsTranslations", "horizontal"),
	QT_TRANSLATE_NOOP("StatsTranslations", "grouped horizontal"),
	QT_TRANSLATE_NOOP("StatsTranslations", "stacked horizontal"),
	QT_TRANSLATE_NOOP("StatsTranslations", "data points"),
	QT_TRANSLATE_NOOP("StatsTranslations", "box-whisker"),
	QT_TRANSLATE_NOOP("StatsTranslations", "piechart"),
};

// Attn: The order must correspond to the enum ChartSortMode
static const char *sortmode_names[] = {
	QT_TRANSLATE_NOOP("StatsTranslations", "Bin"),
	QT_TRANSLATE_NOOP("StatsTranslations", "Count"),
	QT_TRANSLATE_NOOP("StatsTranslations", "Value"),
};

enum class SupportedVariable {
	None,
	Categorical,		// Implies that the variable is binned
	Continuous,		// Implies that the variable is binned
	Numeric
};

static const int ChartFeatureLabels =	  1 << 0;
static const int ChartFeatureLegend =	  1 << 1;
static const int ChartFeatureMedian =	  1 << 2;
static const int ChartFeatureMean =	  1 << 3;
static const int ChartFeatureQuartiles =  1 << 4;
static const int ChartFeatureRegression = 1 << 5;
static const int ChartFeatureConfidence = 1 << 6;

static const struct ChartTypeDesc {
	ChartType id;
	const char *name;
	SupportedVariable var1;
	SupportedVariable var2;
	bool var1Binned, var2Binned, var2HasOperations;
	const std::vector<ChartSubType> subtypes;
	int features;
} chart_types[] = {
	{
		ChartType::ScatterPlot,
		QT_TRANSLATE_NOOP("StatsTranslations", "Scattergraph"),
		SupportedVariable::Continuous,
		SupportedVariable::Numeric,
		false, false, false,
		{ ChartSubType::Dots },
		ChartFeatureRegression | ChartFeatureConfidence
	},
	{
		ChartType::HistogramCount,
		QT_TRANSLATE_NOOP("StatsTranslations", "Histogram"),
		SupportedVariable::Continuous,
		SupportedVariable::None,
		true, false, false,
		{ ChartSubType::Vertical, ChartSubType::Horizontal },
		ChartFeatureLabels | ChartFeatureMedian | ChartFeatureMean
	},
	{
		ChartType::HistogramValue,
		QT_TRANSLATE_NOOP("StatsTranslations", "Histogram"),
		SupportedVariable::Continuous,
		SupportedVariable::Numeric,
		true, false, true,
		{ ChartSubType::Vertical, ChartSubType::Horizontal },
		ChartFeatureLabels
	},
	{
		ChartType::HistogramBox,
		QT_TRANSLATE_NOOP("StatsTranslations", "Histogram"),
		SupportedVariable::Continuous,
		SupportedVariable::Numeric,
		true, false, false,
		{ ChartSubType::Box },
		0
	},
	{
		ChartType::HistogramStacked,
		QT_TRANSLATE_NOOP("StatsTranslations", "Histogram"),
		SupportedVariable::Continuous,
		SupportedVariable::Categorical,
		true, true, false,
		{ ChartSubType::VerticalStacked, ChartSubType::HorizontalStacked },
		ChartFeatureLabels | ChartFeatureLegend
	},
	{
		ChartType::DiscreteScatter,
		QT_TRANSLATE_NOOP("StatsTranslations", "Categorical"),
		SupportedVariable::Categorical,
		SupportedVariable::Numeric,
		true, false, false,
		{ ChartSubType::Dots },
		ChartFeatureQuartiles
	},
	{
		ChartType::DiscreteValue,
		QT_TRANSLATE_NOOP("StatsTranslations", "Categorical"),
		SupportedVariable::Categorical,
		SupportedVariable::Numeric,
		true, false, true,
		{ ChartSubType::Vertical, ChartSubType::Horizontal },
		ChartFeatureLabels
	},
	{
		ChartType::DiscreteCount,
		QT_TRANSLATE_NOOP("StatsTranslations", "Categorical"),
		SupportedVariable::Categorical,
		SupportedVariable::None,
		true, false, false,
		{ ChartSubType::Vertical, ChartSubType::Horizontal },
		ChartFeatureLabels
	},
	{
		ChartType::DiscreteBox,
		QT_TRANSLATE_NOOP("StatsTranslations", "Categorical"),
		SupportedVariable::Categorical,
		SupportedVariable::Numeric,
		true, false, false,
		{ ChartSubType::Box },
		0
	},
	{
		ChartType::Pie,
		QT_TRANSLATE_NOOP("StatsTranslations", "Categorical"),
		SupportedVariable::Categorical,
		SupportedVariable::None,
		true, false, false,
		{ ChartSubType::Pie },
		ChartFeatureLabels | ChartFeatureLegend
	},
	{
		ChartType::DiscreteBar,
		QT_TRANSLATE_NOOP("StatsTranslations", "Barchart"),
		SupportedVariable::Categorical,
		SupportedVariable::Categorical,
		true, true, false,
		{ ChartSubType::VerticalGrouped, ChartSubType::VerticalStacked, ChartSubType::HorizontalGrouped, ChartSubType::HorizontalStacked },
		ChartFeatureLabels | ChartFeatureLegend
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

static const int none_idx = -1; // Special index meaning no second variable

StatsState::StatsState() :
	var1(stats_variables[0]),
	var2(nullptr),
	type(ChartType::DiscreteBar),
	subtype(ChartSubType::Vertical),
	labels(true),
	legend(true),
	median(false),
	mean(false),
	quartiles(true),
	regression(true),
	confidence(true),
	var1Binner(nullptr),
	var2Binner(nullptr),
	var2Operation(StatsOperation::Invalid),
	sortMode1(ChartSortMode::Bin)
{
	validate(true);
}

static StatsState::VariableList createVariableList(const StatsVariable *selected, bool addNone, const StatsVariable *omit)
{
	StatsState::VariableList res;
	res.variables.reserve(stats_variables.size() + addNone);
	res.selected = -1;
	if (addNone) {
		if (selected == nullptr)
			res.selected = (int)res.variables.size();
		res.variables.push_back({ StatsTranslations::tr("none"), none_idx });
	}
	for (int i = 0; i < (int)stats_variables.size(); ++i) {
		const StatsVariable *variable = stats_variables[i];
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
static int toInt(ChartType type, ChartSubType subtype)
{
	return ((int)type << 16) | (int)subtype;
}

static std::pair<ChartType, ChartSubType> fromInt(int id)
{
	return { (ChartType)(id >> 16), (ChartSubType)(id & 0xff) };
}

static ChartValidity variableValidity(StatsVariable::Type type, const StatsBinner *binner, SupportedVariable var, bool binned)
{
	if (!!binner != binned)
		return ChartValidity::Invalid;
	switch (var) {
	default:
	case SupportedVariable::None:
		return ChartValidity::Invalid;	// None has been special cased outside of this function
	case SupportedVariable::Categorical:
		return type == StatsVariable::Type::Continuous || type == StatsVariable::Type::Numeric ?
			ChartValidity::Undesired : ChartValidity::Good;
	case SupportedVariable::Continuous:
		return type == StatsVariable::Type::Discrete ? ChartValidity::Invalid : ChartValidity::Good;
	case SupportedVariable::Numeric:
		return type != StatsVariable::Type::Numeric ? ChartValidity::Invalid : ChartValidity::Good;
	}
}

static ChartValidity chartValidity(const ChartTypeDesc &desc,
				  const StatsVariable *var1, const StatsVariable *var2,
				  const StatsBinner *binner1, const StatsBinner *binner2,
				  StatsOperation operation)
{
	if (!var1)
		return ChartValidity::Invalid; // Huh? We don't support no independent variable

	// Check the first variable
	ChartValidity valid1 = variableValidity(var1->type(), binner1, desc.var1, desc.var1Binned);
	if (valid1 == ChartValidity::Invalid)
		return ChartValidity::Invalid;

	// Then, check the second variable
	if (var2 == nullptr) // Our special marker for "none"
		return desc.var2 == SupportedVariable::None ? valid1 : ChartValidity::Invalid;

	ChartValidity valid2 = variableValidity(var2->type(), binner2, desc.var2, desc.var2Binned);
	if (valid2 == ChartValidity::Invalid)
		return ChartValidity::Invalid;

	// Check whether the chart supports operations.
	if ((operation != StatsOperation::Invalid) != desc.var2HasOperations)
		return ChartValidity::Invalid;

	return valid1 == ChartValidity::Undesired || valid2 == ChartValidity::Undesired ? 
		ChartValidity::Undesired : ChartValidity::Good;
}

// Returns a list of (chart-type, warning) pairs
const std::vector<std::pair<const ChartTypeDesc &, bool>> validCharts(const StatsVariable *var1, const StatsVariable *var2,
								      const StatsBinner *binner1, const StatsBinner *binner2,
								      StatsOperation operation)
{
	std::vector<std::pair<const ChartTypeDesc &, bool>> res;
	res.reserve(std::size(chart_types));
	for (const ChartTypeDesc &desc: chart_types) {
		ChartValidity valid = chartValidity(desc, var1, var2, binner1, binner2, operation);
		if (valid == ChartValidity::Invalid)
			continue;
		res.emplace_back(desc, valid == ChartValidity::Undesired);
	}

	return res;
}

static StatsState::ChartList createChartList(const StatsVariable *var1, const StatsVariable *var2,
					     const StatsBinner *binner1, const StatsBinner *binner2,
					     StatsOperation operation,
					     ChartType selectedType, ChartSubType selectedSubType)
{
	StatsState::ChartList res;
	res.selected = -1;
	for (auto [desc, warn]: validCharts(var1, var2, binner1, binner2, operation)) {
		QString name = StatsTranslations::tr(desc.name);
		for (ChartSubType subtype: desc.subtypes) {
			int id = toInt(desc.id, subtype);
			if (selectedType == desc.id && selectedSubType == subtype)
				res.selected = id;
			QString subtypeName = StatsTranslations::tr(chart_subtype_names[(int)subtype]);
			res.charts.push_back({ name, subtypeName, subtype, toInt(desc.id, subtype), warn });
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

// For non-discrete types propose a "no-binning" option unless this is
// the second variable and has no operations (i.e. is numeric)
static bool noBinningAllowed(const StatsVariable *var, bool second)
{
	if (var->type() == StatsVariable::Type::Discrete)
		return false;
	return !second || var->type() == StatsVariable::Type::Numeric;
}

static StatsState::BinnerList createBinnerList(const StatsVariable *var, const StatsBinner *binner, bool binningAllowed, bool second)
{
	StatsState::BinnerList res;
	res.selected = -1;
	if (!var || !binningAllowed)
		return res;
	std::vector<const StatsBinner *> binners = var->binners();
	res.binners.reserve(binners.size() + 1);
	if (var->type() == StatsVariable::Type::Discrete) {
		if (binners.size() <= 1)
			return res; // Don't show combo boxes for single binners
	} else if (noBinningAllowed(var, second)) {
		if (!second || var->type() == StatsVariable::Type::Numeric) {
			if (!binner)
				res.selected = (int)res.binners.size();
			res.binners.push_back(StatsTranslations::tr("none"));
		}
	}
	for (const StatsBinner *bin: binners) {
		if (bin == binner)
			res.selected = (int)res.binners.size();
		res.binners.push_back(bin->name());
	}
	return res;
}

static StatsState::VariableList createOperationsList(const StatsVariable *var, StatsOperation operation, const StatsBinner *var1Binner)
{
	StatsState::VariableList res;
	res.selected = -1;
	// Operations only possible if the first variable is binned
	if (!var || !var1Binner)
		return res;
	std::vector<StatsOperation> operations = var->supportedOperations();
	if (operations.empty())
		return res;

	res.variables.reserve(operations.size() + 1);

	// Add a "none" entry
	if (operation == StatsOperation::Invalid)
		res.selected = (int)res.variables.size();
	res.variables.push_back({ StatsTranslations::tr("none"), (int)StatsOperation::Invalid });
	for (StatsOperation op: operations) {
		if (op == operation)
			res.selected = (int)res.variables.size();
		res.variables.push_back({ StatsVariable::operationName(op), (int)op });
	}
	return res;
}

static std::vector<StatsState::Feature> createFeaturesList(int chartFeatures, const StatsState &state)
{
	std::vector<StatsState::Feature> res;
	if (chartFeatures & ChartFeatureLabels)
		res.push_back({ StatsTranslations::tr("labels"), ChartFeatureLabels, state.labels });
	if (chartFeatures & ChartFeatureLegend)
		res.push_back({ StatsTranslations::tr("legend"), ChartFeatureLegend, state.legend });
	if (chartFeatures & ChartFeatureMedian)
		res.push_back({ StatsTranslations::tr("median"), ChartFeatureMedian, state.median });
	if (chartFeatures & ChartFeatureMean)
		res.push_back({ StatsTranslations::tr("mean"), ChartFeatureMean, state.mean });
	if (chartFeatures & ChartFeatureQuartiles)
		res.push_back({ StatsTranslations::tr("quartiles"), ChartFeatureQuartiles, state.quartiles });
	if (chartFeatures & ChartFeatureRegression)
		res.push_back({ StatsTranslations::tr("linear regression"), ChartFeatureRegression, state.regression });
	if (chartFeatures & ChartFeatureConfidence)
		res.push_back({ StatsTranslations::tr("95% confidence area"), ChartFeatureConfidence, state.confidence });
	return res;
}

// For creating the sort mode list, the ChartSortMode enum is misused to
// indicate which sort modes are allowed:
// 	bin -> none (list is redundant: only one mode)
//	count -> bin, count
//	value -> bin, count, value
// In principle, the "highest possible" mode is given. If a mode is possible,
// all the lower modes are likewise possible.
static StatsState::VariableList createSortModeList(ChartSortMode allowed, ChartSortMode selectedSortMode)
{
	StatsState::VariableList res;
	res.selected = -1;
	if ((int)allowed <= (int)ChartSortMode::Bin)
		return res;
	for (int i = 0; i <= (int)allowed; ++i) {
		ChartSortMode mode = static_cast<ChartSortMode>(i);
		QString name = StatsTranslations::tr(sortmode_names[i]);
		if (selectedSortMode == mode)
			res.selected = (int)res.variables.size();
		res.variables.push_back({ name, i });
	}

	return res;
}

static StatsState::VariableList createSortModeList1(ChartType type, ChartSortMode selectedSortMode)
{
	ChartSortMode allowed = ChartSortMode::Bin; // Default: no extra sorting
	if (type == ChartType::DiscreteBar || type == ChartType::DiscreteCount || type == ChartType::Pie)
		allowed = ChartSortMode::Count;
	else if (type == ChartType::DiscreteValue)
		allowed = ChartSortMode::Value;
	return createSortModeList(allowed, selectedSortMode);
}

StatsState::UIState StatsState::getUIState() const
{
	UIState res;
	res.var1 = createVariableList(var1, false, nullptr);
	res.var2 = createVariableList(var2, true, var1);
	res.var1Name = var1 ? var1->name() : QString();
	res.var2Name = var2 ? var2->name() : QString();
	res.charts = createChartList(var1, var2, var1Binner, var2Binner, var2Operation, type, subtype);
	res.binners1 = createBinnerList(var1, var1Binner, true, false);
	// Second variable can only be binned if first variable is binned.
	res.binners2 = createBinnerList(var2, var2Binner, var1Binner != nullptr, true);
	res.operations2 = createOperationsList(var2, var2Operation, var1Binner);
	res.features = createFeaturesList(chartFeatures, *this);
	res.sortMode1 = createSortModeList1(type, sortMode1);
	return res;
}

static const StatsBinner *idxToBinner(const StatsVariable *v, int idx, bool second)
{
	if (!v)
		return nullptr;

	// Special case: for non-discrete variables, the first entry means "none".
	if (noBinningAllowed(v, second)) {
		if (idx == 0)
			return nullptr;
		--idx;
	}

	auto binners = v->binners();
	return idx >= 0 && idx < (int)binners.size() ? binners[idx] : 0;
}

void StatsState::var1Changed(int id)
{
	var1 = stats_variables[std::clamp(id, 0, (int)stats_variables.size())];
	validate(true);
}

void StatsState::binner1Changed(int idx)
{
	var1Binner = idxToBinner(var1, idx, false);

	// If the first variable is not binned, the second variable must be of the "numeric" type.
	if(!var1Binner && (!var2 || var2->type() != StatsVariable::Type::Numeric)) {
		// Find first variable that is numeric, but not the same as the first
		auto it = std::find_if(stats_variables.begin(), stats_variables.end(),
				       [v1 = var1] (const StatsVariable *v)
				       { return v != v1 && v->type() == StatsVariable::Type::Numeric; });
		var2 = it != stats_variables.end() ? *it : nullptr;
	}

	validate(false);
}

void StatsState::var2Changed(int id)
{
	// The "none" variable is represented by a nullptr
	var2 = id == none_idx ? nullptr
			      : stats_variables[std::clamp(id, 0, (int)stats_variables.size())];
	validate(true);
}

void StatsState::binner2Changed(int idx)
{
	var2Binner = idxToBinner(var2, idx, true);

	// We do not support operations and binning at the same time.
	if (var2Binner)
		var2Operation = StatsOperation::Invalid;

	validate(false);
}

void StatsState::var2OperationChanged(int id)
{
	var2Operation = (StatsOperation)id;

	// We do not support operations and binning at the same time.
	if (var2Operation != StatsOperation::Invalid)
		var2Binner = nullptr;

	validate(false);
}

void StatsState::sortMode1Changed(int id)
{
	sortMode1 = (ChartSortMode)id;
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
	else if (id == ChartFeatureLegend)
		legend = state;
	else if (id == ChartFeatureMedian)
		median = state;
	else if (id == ChartFeatureMean)
		mean = state;
	else if (id == ChartFeatureQuartiles)
		quartiles = state;
	else if (id == ChartFeatureRegression)
		regression = state;
	else if (id == ChartFeatureConfidence)
		confidence = state;
}

// Creates the new chart-type from the current chart-type and a list of possible chart types.
// If the flag "varChanged" is true, the current chart-type will be changed if the
// current chart-type is undesired.
static const ChartTypeDesc &newChartType(ChartType type,
					 const std::vector<std::pair<const ChartTypeDesc &, bool>> &charts,
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

static const StatsBinner *getFirstBinner(const StatsVariable *var)
{
	if (!var)
		return nullptr;
	auto binners = var->binners();
	return binners.empty() ? nullptr : binners.front();
}

static void validateBinner(const StatsBinner *&binner, const StatsVariable *var, bool second)
{
	if (!var) {
		binner = nullptr;
		return;
	}

	bool noneOk = noBinningAllowed(var, second);
	if (noneOk & !binner)
		return;

	auto binners = var->binners();
	if (std::find(binners.begin(), binners.end(), binner) != binners.end())
		return;

	// For now choose the first binner or no binner if this is a non-discrete
	// variable. However, we might try to be smarter here and adapt to the
	// given screen size and the estimated number of bins.
	binner = binners.empty() || noneOk ? nullptr : binners[0];
}

static void validateOperation(StatsOperation &operation, const StatsVariable *var, const StatsBinner *var1Binner)
{
	// 1) No variable, no operation.
	// 2) We allow operations only if the first variable is binned.
	if (!var) {
		operation = StatsOperation::Invalid;
		return;
	}

	std::vector<StatsOperation> ops = var->supportedOperations();
	if (std::find(ops.begin(), ops.end(), operation) != ops.end())
		return;

	operation = StatsOperation::Invalid;
}

// The var changed variable indicates whether this function is called
// after a variable change or a change of the chart type. In the
// former case, the chart type is switched, if it is not recommended.
// In the latter case, the user explicitly chose a non-recommended type,
// so let's use that.
void StatsState::validate(bool varChanged)
{
	// We need at least one variable
	if (!var1)
		var1 = stats_variables[0];

	// Take care that we don't plot a variable against itself.
	if (var1 == var2)
		var2 = nullptr;

	validateBinner(var1Binner, var1, false);

	// If there is no second variable or the second variable is not
	// "numeric", the first variable must be binned.
	if ((!var2 || var2->type() != StatsVariable::Type::Numeric) && !var1Binner)
		var1Binner = getFirstBinner(var1);

	// Check that the binners and operation are valid
	if (!var1Binner) {
		var2Binner = nullptr;	// Second variable can only be binned if first variable is binned.
		var2Operation = StatsOperation::Invalid;
	}
	validateBinner(var2Binner, var2, true);
	validateOperation(var2Operation, var2, var1Binner);

	// Let's see if the currently selected chart is one of the valid charts
	auto charts = validCharts(var1, var2, var1Binner, var2Binner, var2Operation);

	if (charts.empty()) {
		// Ooops. No valid chart with these settings. The validation should be improved.
		type = ChartType::Invalid;
		return;
	}

	const ChartTypeDesc &desc = newChartType(type, charts, varChanged);
	type = desc.id;

	// Check if the current subtype is supported by the chart
	if (std::find(desc.subtypes.begin(), desc.subtypes.end(), subtype) == desc.subtypes.end())
		subtype = desc.subtypes.empty() ? ChartSubType::Horizontal : desc.subtypes[0];

	chartFeatures = desc.features;
	// Median and mean currently only if the first variable is numeric
	if (!var1 || var1->type() != StatsVariable::Type::Numeric)
		chartFeatures &= ~(ChartFeatureMedian | ChartFeatureMean);

	// By default, sort according to the used bin. Only for pie charts,
	// sort by count, if the binning is on a categorical variable.
	if (varChanged) {
		sortMode1 = type == ChartType::Pie &&
			    var1->type() == StatsVariable::Type::Discrete ? ChartSortMode::Count :
									    ChartSortMode::Bin;
	}
}
