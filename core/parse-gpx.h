// SPDX-License-Identifier: GPL-2.0
#ifndef PARSE_GPX_H
#define PARSE_GPX_H

#include <QString>

struct dive_coords {         // This structure holds important information after parsing the GPX file:
	time_t start_dive;            // Start time of the current dive, obtained using current_dive (local time)
	time_t end_dive;              // End time of current dive (local time)
	time_t start_track;           // Start time of GPX track (UTC)
	time_t end_track;             // End time of GPX track (UTC)
	double lon;                   // Longitude of the first trackpoint after the start of the dive
	double lat;                   // Latitude of the first trackpoint after the start of the dive
	int64_t settingsDiff_offset;  // Local time difference between dive computer and GPS equipment
	int64_t timeZone_offset;      // UTC international time zone offset of dive site
};

int getCoordsFromGPXFile(dive_coords *coords, const QString &fileName);

#endif
