// SPDX-License-Identifier: GPL-2.0
// Describes the current state of the statistics widget
// (selected variables, chart type, etc.) and is the
// interface between UI and plotting code.
#ifndef STATS_STATE_H
#define STATS_STATE_H

#include <vector>
#include <QString>

enum class ChartType {
	DiscreteBar,
	DiscreteValue,
	DiscreteCount,
	DiscreteBox,
	DiscreteScatter,
	Pie,
	HistogramCount,
	HistogramValue,
	HistogramBox,
	HistogramStacked,
	ScatterPlot,
	Invalid
};

enum class ChartSubType {
	Vertical = 0,
	VerticalGrouped,
	VerticalStacked,
	Horizontal,
	HorizontalGrouped,
	HorizontalStacked,
	Dots,
	Box,
	Pie,
	Count
};

struct ChartTypeDesc; // Internal implementation detail
struct StatsVariable;
struct StatsBinner;
enum class StatsOperation : int;

struct StatsState {
public:
	StatsState();
	int setFirstAxis();
	int setSecondAxis();

	struct Variable {
		QString name;
		int id;
	};
	struct VariableList {
		std::vector<Variable> variables;
		int selected;
	};
	struct Chart {
		QString name;
		QString subtypeName;
		ChartSubType subtype;
		int id;
		bool warning;		// Not recommended for that combination
	};
	struct ChartList {
		std::vector<Chart> charts;
		int selected;
	};
	struct BinnerList {
		std::vector<QString> binners;
		int selected;
	};
	struct Feature {
		QString name;
		int id;
		bool selected;
	};
	struct UIState {
		VariableList var1;
		VariableList var2;
		QString var1Name;
		QString var2Name;
		ChartList charts;
		std::vector<Feature> features;
		BinnerList binners1;
		BinnerList binners2;
		// Currently, operations are only supported on the second variable
		// This reuses the variable list - not very nice.
		VariableList operations2;
	};
	UIState getUIState() const;

	// State changers
	void var1Changed(int id);
	void var2Changed(int id);
	void chartChanged(int id);
	void binner1Changed(int id);
	void binner2Changed(int id);
	void var2OperationChanged(int id);
	void featureChanged(int id, bool state);

	const StatsVariable *var1;	// Independent variable
	const StatsVariable *var2;	// Dependent variable (nullptr: count)
	ChartType type;
	ChartSubType subtype;
	bool labels;
	bool legend;
	bool median;
	bool mean;
	bool quartiles;
	bool regression;
	bool confidence;
	const StatsBinner *var1Binner;	// nullptr: undefined
	const StatsBinner *var2Binner;	// nullptr: undefined
	StatsOperation var2Operation;
private:
	void validate(bool varChanged);
	int chartFeatures;
};

#endif
