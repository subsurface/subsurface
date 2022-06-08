- core: when modifying cylinders across multiple dives, match cylinder number before comparing type
- core: merge all properties in a dive, including current, waveheight, etc
- core: prevent crash when merging dives without cylinders (as we might get when importing from divelogs.de)
- core: work around bug in TecDiving dive computer reporting spurious 0 deg C water temperature in first sample
- core: correctly parse DC_FIELD_SALINITY response; fixes incorrect water type with some dive computers, including the Mares Smart
- Desktop: Allow more than one media file to be imported from web
- undo: Clear undo stack when the current file is closed
- dive computer support
 - Garmin: correctly deal with short format filenames
 - Garmin: correctly parse dive mode
 - Add support for the Seac Screen and Action
 - Add support for the Cressi Michelangelo


---
* Always add new entries at the very top of this file above other existing entries and this note.
* Use this layout for new entries: `[Area]: [Details about the change] [reference thread / issue]`
# vim: textwidth=100
