- Store the gas switch depth of a cylinder from the planner in the logbook
  file or git storage. No more need to reenter this value on replanning.
- Improved handling of different information (divemaster, buddy, suit, notes)
  when merging two dives.
- Limit min. GFlow to 10 and min. GFhigh to 40 in preferences for profile
  and planner
- Fix issues related to debug logging on Windows
- Add "Bluetooth mode" in the BT selection dialog: Auto, LE, Classical
- Correct display of cylinder pressures for merged dives
- Allow user defined cylinders as default in preferences (#821)
- mobile: fix black/white switch in splash screen (#531)
- UI: tag editing. Comma entry shows all tags (again) (#605)
- mobile: enable auto completion for dive site entry (#546)
- Printing: the bundled templates are now read-only and are always overwritten
  by the application. The first time the user runs this update, backup files
  of the previous templates would be created (#847)
- Fix issues with filters not updating after changes to the dive list
  (#551, #675)
- map-widget: allow updating coordinates on the map when the user
  is editing a dive site by pressing Enter or clicking a "flag" button
- map-widget: prevent glitches when the user is interacting with the map
  while animations are in progress
