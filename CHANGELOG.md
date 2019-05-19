- Desktop: Add undo functionality for dive computer movement and deletion
- Import: Small enhancements on Suunto SDE import
- Desktop: Add import dive site menu option and site selection dialog
- Core: Sort dives by number if at the same date
- Core: fix bug in get_distance() to correctly compute spherical distance
- Desktop: For videos, add save data export as subtitle file
- Desktop: make dive sites 1st class citizens with their own dive site table
- Desktop: only show dives at the dive sites selected in dive site table
- Desktop: add undo functionality to edit operations and remove 'edited' state;
  this fundamentally changes the way dives are edited and manually added.
  (partial implementation, tanks and weights are missing)
- Desktop: Add export option for dive sites
- Import: Initial support for importing Mares log software
- Export option for profile data
- Desktop: Splitting of individual dive computers into distinct dives
- Planner: Allow for a final segment at the surface to display further desaturation
- Desktop: Add stats by depth and temperature ranges to yearly stats [#1996]
- Desktop: make sure cloud storage email addresses are lower case only
- Desktop: Fix editing of dive-time [#1975]
- Bluetooth: only show recognized dive computers by default
- Desktop: Add export option for profile picture [#1962]
- Export: fix picture thumbnails [#1963]
- Desktop: remove support for the "Share on Facebook" feature.
  Rationale: It is fairly easy to share images on Facebook, thus it was decided
  that this feature is redundant and should be removed from Subsurface.
- Show surface gradient factor in infobox
- Planner: Add UI element for bailout planning for rebreather dives
- Allow to filter for logged/planned dives
- New LaTeX export option
- Core, Windows: fix a bug related to non-ASCII characters in user names
- Core: Merge overlapping trips on import
- Desktop: Make dive import and download undoable
- Desktop: Add media to closest dive, not all selected dives
- Include average max depth in statistics
- Fix bug in cloud save after removing dives from a trip
- Dive: Perform more accurate OTU calculations, and include
  OTU calculations for rebreather dives [#1851 & #1865].
- Mobile: UI for copy-paste
- Mobile: add initial copy-paste support
---
* Always add new entries at the very top of this file above other existing entries and this note.
* Use this layout for new entries: `[Area]: [Details about the change] [reference thread / issue]`
