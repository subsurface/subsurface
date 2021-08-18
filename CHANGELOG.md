- replace dive computer management with a new implementation, this fixes rare cases where
  Subsurface unnecessarily download already downloaded dives, but also means
  that some older style dive computer can no longer have nicknames
- avoid potential crash when switching divelogs
- add GPS fix downloaded from a dive comuter to existing dive site
- fix broken curser left/right shortcut for showing multiple dive computers
- allow editing the profile for dives imported without samples (via CSV)
- fix potential crash (and other errors) importing DL7 dives
- add detection for Aladin A2 as BLE dive computer
- improve printout scaling for event icons
- support exporting unused cylinders to divelogs.de
- improve handling of CNS values when combining multiple dive computers

---
* Always add new entries at the very top of this file above other existing entries and this note.
* Use this layout for new entries: `[Area]: [Details about the change] [reference thread / issue]`
# vim: textwidth=100
