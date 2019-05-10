// SPDX-License-Identifier: GPL-2.0
#include "applicationstate.h"

static ApplicationState appState = (ApplicationState)-1; // Set to an invalid value

ApplicationState getAppState()
{
	return appState;
}

void setAppState(ApplicationState state)
{
	appState = state;
}

