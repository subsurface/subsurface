// SPDX-License-Identifier: GPL-2.0
#include "statsselection.h"
#include "core/dive.h"
#include "core/selection.h"

#include <algorithm>

void processSelection(std::vector<dive *> dives, SelectionModifier modifier)
{
	std::vector<dive *> selected;

	if (modifier.ctrl) {
		// When shift is pressed, add the items under the mouse to the selection
		// or, if all items under the mouse are selected, remove them.
		selected = getDiveSelection();
		bool allSelected = std::all_of(dives.begin(), dives.end(),
					       [] (const dive *d) { return d->selected; });
		if (allSelected) {
			// Remove items under cursor from selection. This could be made more efficient.
			for (const dive *d: dives) {
				auto it = std::find(selected.begin(), selected.end(), d);
				if (it != selected.end()) {
					// Move last element to deselected element. If this already was
					// the last element, this is a no-op. Then, chop off last element.
					*it = selected.back();
					selected.pop_back();
				}
			}
		} else {
			// Add items under cursor to selection
			selected.reserve(dives.size() + selected.size());
			for (dive *d: dives) {
				if (std::find(selected.begin(), selected.end(), d) == selected.end())
					selected.push_back(d);
			}
		}
	} else {
		selected = std::move(dives);
	}

	setSelection(selected, selected.empty() ? nullptr : selected.front());
}
