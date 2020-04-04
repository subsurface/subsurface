Mobile: fix no-cloud to cloud transition
Mobile: remove locking of data storage access
Mobile: performance improvements for dive list
Mobile: user notification of progress during startup
Mobile: ensure filter text field has focus when opened
Mobile: dive summary: make time drop downs smaller
Mobile: close menus with Android back button on dive details
Mobile: add ability to toggle dive invalid flag from dive details context menu
Mobile: calculate dive summary statistics in 64 bit values even on 32bit ARM
Mobile: fix strange issue where sometimes the keyboard would pop up in dive view mode
Mobile: remove 'map it' button on dive view page - it was redundant and took space from location field
Mobile: show better placeholder text while processing dive list
Mobile: Android: don't quit the app when trying to close a drawer/menu with back button
Mobile: make sure filter header and virtual keyboard are shown when tapping on filter icon
Mobile: fix issue where in some circumstances changes weren't written to storage
Mobile: fix issues with filtering while editing dives
Desktop: implement dive invalidation
Desktop: fix tab-order in filter widget
Desktop: implement fulltext search
Desktop: add starts-with and exact filter modes for textual search
Desktop: ignore dive sites without location in proximity search
Desktop: add the ability to modify dive salinity
Profile: Add current GF to infobox
Desktop: increase speed of multi-trip selection
Desktop/Linux: fix issue with Linux AppImage failing to communicate with Bluetooth dive computers [#2370]
Desktop: allow copy&pasting of multiple cylinders [#2386]
Desktop: don't output random SAC values for cylinders without data [#2376]
Planner: Add checkbox on considering oxygen narcotic
Planner: Improve rounding of stop durations in planner notes
Desktop: register changes when clicking "done" on dive-site edit screen
Core: support dives with no cylinders
Core: remove restriction on number of cylinders
Desktop: make dive replanning undoable
Desktop: update statistics tab on undo or redo
Planner: update dive details when replanning dive [#2280]
Export: when exporting dive sites in dive site mode, export selected dive sites [#2275]
libdivecomputer:
- Add support for the Oceanic Geo 4.0
- clean up Shearwater tank pressure handling
- minor fixlets
---
* Always add new entries at the very top of this file above other existing entries and this note.
* Use this layout for new entries: `[Area]: [Details about the change] [reference thread / issue]`
# vim: textwidth=100
