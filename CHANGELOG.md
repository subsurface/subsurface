- Import: Initial support for importing Mares log software
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
