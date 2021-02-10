// SPDX-License-Identifier: GPL-2.0
// Functions and data structures handling dive selections.

#ifndef STATS_SELECTION_H
#define STATS_SELECTION_H

#include <vector>

struct dive;

struct SelectionModifier {
	unsigned int ctrl : 1;
	unsigned int shift : 1;
	// Note: default member initializers for bit-fields becomes available with C++20.
	// Therefore, for now an inline constructor.
	SelectionModifier() : ctrl(0), shift(0) {}
};

void processSelection(std::vector<dive *> dives, SelectionModifier modifier);

#endif
