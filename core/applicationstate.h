// SPDX-License-Identifier: GPL-2.0
#ifndef APPLICATIONSTATE_H
#define APPLICATIONSTATE_H

// By using an enum class, the enum entries don't polute the global namespace.
// Moreover, they are strongly typed. This means that they are not auto-converted
// to integer types if e.g. used as array-indices.
enum class ApplicationState {
	Default,
	EditDive,
	PlanDive,
	EditPlannedDive,
	EditDiveSite,
	FilterDive,
	Count
};

ApplicationState getAppState();
void setAppState(ApplicationState state);

#endif
