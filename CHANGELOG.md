desktop: fix gas switches in UDDF exports
core: allow of up to 6 O2 sensors (and corresponding voting logic)
desktop: add divemode as a possible dive list column
profile-widget: Now zomed in profiles can be panned with horizontal scroll.
desktop: hide only events with the same severity when 'Hide similar events' is used
equipment: mark gas mixes reported by the dive computer as 'inactive' as 'not used'
equipment: include unused cylinders in merged dive if the preference is enabled
equipment: fix bug showing the first diluent in the gas list as 'used' for CCR dives
desktop: added button to hide the infobox in the dive profile
desktop: use persisted device information for the dive computer configuration
core: fix bug using salinity and pressure values in mbar <-> depth conversion
export: fix bug resulting in invalid CSV for '""' in 'CSV summary dive details'
desktop: add support for multiple tanks to the profile ruler
export: change format produced by 'CSV summary dive details' from TSV (tab separated) to CSV
desktop: add function to merge dive site into site selected in list
import: add option to synchronise dive computer time when downloading dives
desktop: fix salinity combo/icon when DC doesnt have salinity info
core: fix bug when save sea water salinity given by DC
desktop: add option to force firmware update on OSTC4
desktop: add column for dive notes to the dive list table
desktop: Add an option for printing in landscape mode
desktop: fix bug when printing a dive plan with multiple segments
desktop: fix remembering of bluetooth address for remembered dive computers (not MacOS)
desktop: fix bug in bailout gas selection for CCR dives
desktop: fix crash on cylinder update of multiple dives
desktop: use dynamic tank use drop down in equipment tab and planner
desktop: fix brightness configuration for OSTC4
equipment: Use 'diluent' as default gas use type if the dive mode is 'CCR'
htmlexport: fix search in HTML export
htmlexport: fix diveguide display
statistics: fix value axis for degenerate value ranges
profile: Show correct gas density when in CCR mode
statistics: show correct color of selected scatter items when switching to unbinned mode
statistics: fix display of month number in continuous date axis
statistics: fix range of continuous date axis
desktop: fix dive time display in time shift dialog
libdivecomputer: garmin: relax string parsing sanity checks
libdivecomputer: add Cressi Donatello, Scubapro G2 TEK, Oceanic Geo Air, Scorpena Alpha

---
* Always add new entries at the very top of this file above other existing entries and this note.
* Use this layout for new entries: `[Area]: [Details about the change] [reference thread / issue]`
# vim: textwidth=100
