// SPDX-License-Identifier: GPL-2.0
#include "core/parse-gpx.h"
#include "core/subsurface-time.h"
#include "core/namecmp.h"
#include <QFile>
#include <QXmlStreamReader>

// Find the coordinates at the time specified in coords.start_dive
// by searching the gpx file "fileName". Here is a typical trkpt element in GPX:
// <trkpt lat="-26.84" lon="32.88"><ele>-53.7</ele><time>2017-08-06T04:56:42Z</time></trkpt>
int getCoordsFromGPXFile(struct dive_coords *coords, const QString &fileName)
{
	struct tm tm1;
	time_t trkpt_time = 0;
	time_t divetime;
	int64_t time_offset = coords->settingsDiff_offset + coords->timeZone_offset;
	double lon = 0, lat = 0;
	int line = 0;
	bool first_line = true;
	bool found = false;
	bool trkpt_found = false;
	divetime = coords->start_dive;
	QFile gpxFile;
	gpxFile.setFileName(fileName);
	if (!gpxFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
		QByteArray local8bitBAString1 = fileName.toLocal8Bit();
		char *fname = local8bitBAString1.data();   // convert QString to a C string fileName
		fprintf(stderr, "GPS file open error: file name = %s\n", fname);
		return 1;
	}

#ifdef GPSDEBUG
	struct tm time; // decode the time of start of dive:
	utc_mkdate(divetime, &time);
	int dyr,dmon,dday,dhr,dmin;
	dyr = time.tm_year;
	dmon = time.tm_mon;
	dday = time.tm_mday;
	dhr = time.tm_hour;
	dmin = time.tm_min;
#endif

	QXmlStreamReader gpxReader(&gpxFile);
	while (!gpxReader.atEnd()) {
		gpxReader.readNext();
		if (gpxReader.isStartElement()) {
			if (nameCmp(gpxReader, "trkpt") == 0) {
				trkpt_found = true;
				line++;
				foreach (const QXmlStreamAttribute &attr, gpxReader.attributes()) {
					if (attr.name().toString() == QLatin1String("lat"))
						lat = attr.value().toString().toDouble();
					else if (attr.name().toString() == QLatin1String("lon"))
						lon = attr.value().toString().toDouble();
				}
			}
			if (nameCmp(gpxReader, "time") == 0 && trkpt_found) {  // Ignore the <time> element in the GPX file header
				QString dateTimeString = gpxReader.readElementText();
				bool ok;
				tm1.tm_year = dateTimeString.left(4).toInt(&ok, 10);  // Extract the date/time components:
				tm1.tm_mon  = dateTimeString.mid(5,2).toInt(&ok,10) - 1;
				tm1.tm_mday = dateTimeString.mid(8,2).toInt(&ok,10);
				tm1.tm_hour = dateTimeString.mid(11,2).toInt(&ok,10);
				tm1.tm_min  = dateTimeString.mid(14,2).toInt(&ok,10);
				tm1.tm_sec  = dateTimeString.mid(17,2).toInt(&ok,10);
				trkpt_time = utc_mktime(&tm1) + time_offset;
				if (first_line) {
					first_line = false;
					coords->start_track = trkpt_time;   // Local time of start of GPS track
				}
				if (trkpt_time >= divetime && found == false) {   // This GPS local time corresponds to the start time of the dive
					coords->lon = lon; // save the coordinates
					coords->lat = lat;
					found = true;
				}

#ifdef GPSDEBUG
				utc_mkdate(trkpt_time, &time); // print coordinates and time of each trkpt element of the GPX file as well as dive start time
				fprintf(stderr, " %02d: lat=%f lon=%f timestamp=%ld (%ld) %02d/%02d/%02d %02d:%02d  dt=%ld  %02d/%02d/%02d %02d:%02d\n", line, lat,
				lon, trkpt_time, time_offset, time.tm_year, time.tm_mon+1, time.tm_mday, time.tm_hour, time.tm_min, divetime, dyr, dmon+1, dday,dhr, dmin);
#endif

			}
		}
	} // while !at.End() // This loop executes until EOF causes a break out of the loop
	coords->end_track = trkpt_time;  // This is the local time of the end of the GPS track
	gpxFile.close();
	return 0;
}
