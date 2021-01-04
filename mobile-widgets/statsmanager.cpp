// SPDX-License-Identifier: GPL-2.0
#include "statsmanager.h"

StatsManager::StatsManager() : view(nullptr)
{
	// Test: show some random data. Let's see what happens.
	state.var1Changed(2);
	state.var2Changed(3);
	state.binner2Changed(2);
	updateUi();
}

StatsManager::~StatsManager()
{
}

void StatsManager::init(StatsView *v, QObject *o)
{
	if (!v)
		fprintf(stderr, "StatsManager::init(): no StatsView - statistics will not work.\n");
	view = v;
}

void StatsManager::doit()
{
	if (!view)
		return;
	view->plot(state);
}

static void setVariableList(const StatsState::VariableList &list, QStringList &stringList)
{
	stringList.clear();
	for (const StatsState::Variable &v: list.variables)
		stringList.push_back(v.name);
}

static void setBinnerList(const StatsState::BinnerList &list, QStringList &stringList)
{
	stringList.clear();
	for (const QString &v: list.binners)
		stringList.push_back(v);
}

void StatsManager::updateUi()
{
	uiState = state.getUIState();
	setVariableList(uiState.var1, var1List);
	setBinnerList(uiState.binners1, binner1List);
	setVariableList(uiState.var2, var2List);
	setBinnerList(uiState.binners2, binner2List);
	var1ListChanged();
	binner1ListChanged();
	var2ListChanged();
	binner2ListChanged();

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
