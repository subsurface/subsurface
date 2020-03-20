Mobile: support marking dives as invalid
Desktop: implement dive invalidation
Mobile-android: remove libusb/FTDI support (largely non-functional)
Mobile-android: Continue download after obtaining USB permission
Mobile-android: Add usb-serial-for-android driver support
Mobile: fix missing download info on retry
Mobile: enable profile zoom and pan
Mobile: add people- and tags-filter modes
Desktop: fix tab-order in filter widget
Desktop: implement fulltext search
Mobile: fix saving newly applied GPS fixes to storage
Desktop: add starts-with and exact filter modes for textual search
Mobile: add option to only show one column in portrait mode
Mobile: fix potential crash when adding / editing dives
Mobile: automatically scroll the dive edit screen so that the notes edit cursor stays visible
Desktop: ignore dive sites without location in proximity search
Mobile: add personalized option for units
Mobile: add Dive Summary page
Mobile: option to reset default cylinder
Mobile: new layout for Settings page
Desktop: add the ability to modify dive salinity
Mobile: add menu item for cloud password reset
Mobile: add DivePlanner setup, as first part of a mobile diveplanner
Mobile: secure user selects operational mode, before leaving login
Mobile: use of qml compiler, smaller footprint and faster UI
Mobile: support for "upload to divelogs.de"
mobile: add support for upload to dive-share.com
Profile: Add current GF to infobox
Mobile: (desktop only) new switch --testqml, allows to use qml files instead of resources.
Desktop: increase speed of multi-trip selection
Mobile: ensure that all BT/BLE flavors of the OSTC are recognized as dive computers [#2358]
mobile-widgets: Add export function
Desktop/Linux: fix issue with Linux AppImage failing to communicate with Bluetooth dive computers [#2370]
Desktop: allow copy&pasting of multiple cylinders [#2386]
Desktop: don't output random SAC values for cylinders without data [#2376]
Mobile: add menu entry to start a support email, using native email client
Mobile: fix subtle potential new data loss bug in first session connecting to a cloud account
Planner: Add checkbox on considering oxygen narcotic
Planner: Improve rounding of stop durations in planner notes
Desktop: register changes when clicking "done" on dive-site edit screen
Mobile: re-enable GPS location service icon in global drawer
Core: support dives with no cylinders
Core: remove restriction on number of cylinders
Mobile: add support for editing the dive number of a dive
Desktop: make dive replanning undoable
Desktop: update statistics tab on undo or redo
Planner: update dive details when replanning dive [#2280]
Export: when exporting dive sites in dive site mode, export selected dive sites [#2275]
Mobile: fix several bugs in the handling of incorrect or unverified cloud credentials
Mobile: try to adjust the UI for cases with large default font relative to screen size
libdivecomputer:
- Add support for the Oceanic Geo 4.0
- clean up Shearwater tank pressure handling
- minor fixlets
---
* Always add new entries at the very top of this file above other existing entries and this note.
* Use this layout for new entries: `[Area]: [Details about the change] [reference thread / issue]`
