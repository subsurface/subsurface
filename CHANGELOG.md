- Windows: add experimental support for BTLE dive computers.
  For more information have a look at section 5.2.2 of the user
  manual ("On Windows").
- Import: Fix import of CSV files containing quotes
- Desktop: Show coordinates if no tags from reverse geo lookup
- Desktop: Sort on date in case of equal fields
- Desktop: Indicate sort direction in list header
- Desktop: Use defined sort-order when switching sort column
- Fix a bug in planner for dives with manual gas changes.
- Mobile: add initial copy-paste support
- Mobile: add full text filtering of the dive list
- Desktop: don't add dive-buddy or dive-master when tabbing through fields
- Filter: don't recalculate all filters on dive list-modifying operations
- Desktop: don't recalculate full dive list on dive list-modifying operations
- Undo: implement undo/redo for all dive list-modifying operations
- Mobile: preserve manual/auto sync with cloud [#1725]
- Desktop: don't show "invalid dive site" debugging message
- Map: zoom on dive site when selecting items in dive site list
- Performance: calculate full statistics only when needed
- HTML export: write statistics of exported dives only
- Desktop: fix calculation of surface interval in the case of overlappimg dives
- Desktop: import Poseidon MkIV via dive log import dialog
---
* Always add new entries at the very top of this file above other existing entries and this note.
* Use this layout for new entries: `[Area]: [Details about the change] [reference thread / issue]`
