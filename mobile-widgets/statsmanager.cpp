// SPDX-License-Identifier: GPL-2.0
#include "statsmanager.h"

StatsManager::StatsManager() : view(nullptr)
{
	// Test: show some random data. Let's see what happens.
	state.var1Changed(2);
	state.var2Changed(3);
	state.binner2Changed(2);
}

StatsManager::~StatsManager()
{
}

void StatsManager::init(StatsView *v)
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
