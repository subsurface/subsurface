// SPDX-License-Identifier: GPL-2.0
#include "statsmanager.h"
#include "themeinterface.h"
#include "stats/chartlistmodel.h"
#include "core/errorhelper.h"

StatsManager::StatsManager() : view(nullptr), charts(nullptr), themeInitialized(false)
{
	updateUi();
}

StatsManager::~StatsManager()
{
}

void StatsManager::init(StatsView *v, ChartListModel *m)
{
	if (!v)
		report_info("StatsManager::init(): no StatsView - statistics will not work.");
	if (!m)
		report_info("StatsManager::init(): no ChartListModel - statistics will not work.");
	view = v;
	charts = m;

	connect(ThemeInterface::instance(), &ThemeInterface::currentThemeChanged,
		this, &StatsManager::themeChanged);
}

void StatsManager::doit()
{
	updateUi();
}

static void setVariableList(const StatsState::VariableList &list, QStringList &stringList, int &idx)
{
	stringList.clear();
	for (const StatsState::Variable &v: list.variables)
		stringList.push_back(v.name);
	idx = list.selected;
}

static void setBinnerList(const StatsState::BinnerList &list, QStringList &stringList, int &idx)
{
	stringList.clear();
	for (const QString &v: list.binners)
		stringList.push_back(v);
	idx = list.selected;
}

void StatsManager::updateUi()
{
	uiState = state.getUIState();
	setVariableList(uiState.var1, var1List, var1Index);
	setBinnerList(uiState.binners1, binner1List, binner1Index);
	setVariableList(uiState.var2, var2List, var2Index);
	setBinnerList(uiState.binners2, binner2List, binner2Index);
	setVariableList(uiState.operations2, operation2List, operation2Index);
	setVariableList(uiState.sortMode1, sortMode1List, sortMode1Index);
	var1ListChanged();
	binner1ListChanged();
	var2ListChanged();
	binner2ListChanged();
	operation2ListChanged();
	sortMode1ListChanged();
	var1IndexChanged();
	binner1IndexChanged();
	var2IndexChanged();
	binner2IndexChanged();
	operation2IndexChanged();
	sortMode1IndexChanged();

	if (view && !std::exchange(themeInitialized, true))
		themeChanged();
	if (charts)
		charts->update(uiState.charts);
	if (view)
		view->plot(state);
}

void StatsManager::var1Changed(int idx)
{
	if (uiState.var1.variables.empty())
		return;
	idx = std::clamp(idx, 0, (int)uiState.var1.variables.size());
	state.var1Changed(uiState.var1.variables[idx].id);
	updateUi();
}

void StatsManager::var1BinnerChanged(int idx)
{
	state.binner1Changed(idx);
	updateUi();
}

void StatsManager::var2Changed(int idx)
{
	if (uiState.var2.variables.empty())
		return;
	idx = std::clamp(idx, 0, (int)uiState.var2.variables.size());
	state.var2Changed(uiState.var2.variables[idx].id);
	updateUi();
}

void StatsManager::var2BinnerChanged(int idx)
{
	state.binner2Changed(idx);
	updateUi();
}

void StatsManager::var2OperationChanged(int idx)
{
	if (uiState.operations2.variables.empty())
		return;
	idx = std::clamp(idx, 0, (int)uiState.operations2.variables.size());
	state.var2OperationChanged(uiState.operations2.variables[idx].id);
	updateUi();
}

void StatsManager::sortMode1Changed(int idx)
{
	if (uiState.sortMode1.variables.empty())
		return;
	idx = std::clamp(idx, 0, (int)uiState.sortMode1.variables.size());
	state.sortMode1Changed(uiState.sortMode1.variables[idx].id);
	updateUi();
}

void StatsManager::setChart(int idx)
{
	state.chartChanged(idx);
	updateUi();
}

void StatsManager::themeChanged()
{
	if (!view)
		return;

	// We could just make currentTheme accessible instead of
	// using Qt's inane propertySystem. Whatever.
	QString theme = ThemeInterface::instance()->property("currentTheme").toString();
	view->setTheme(theme == "Dark");
}
