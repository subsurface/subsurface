// SPDX-License-Identifier: GPL-2.0
#include "core/parse-gpx.h"
#include "core/dive.h"

// TODO: instead of manually parsing the XML, we should consider using
//       one of the two XML frameworks that we already use elsewhere in
//       the code

//  Read text from the present position in the file until
//  the first 'delim' character is encountered.
static int getSubstring(QFile *file, QString *bufptr, char delim)
{
	char c;
	bufptr->clear();
	do {
		if (file->read(&c, 1) <= 0) // EOF
			return 1;
		if (c == delim) break;
		bufptr->append(QChar(c));
	} while (c != delim);
	return 0;
}

// Find the next occurence of a specified target GPX element in the file,
// characerised by a "<xxx " or "<xxx>" character sequence.
// 'target' specifies the name of the element searched for.
// termc is the ending character of the element name search = '>' or ' '.
static int findXmlElement(QFile *fileptr, QString target, QString *bufptr, char termc)
{
	bool match = false;
	char c;
	char skipdelim = (termc == ' ') ? '>' : ' ';
	do {    // Skip input until first start new of element (<) is encountered:
		if (getSubstring(fileptr, bufptr, '<'))
			return 1;  // EOF
		bufptr->clear();
		do {  // Read name of following element and store it in buf
			if (fileptr->read(&c, 1) <= 0)  // EOF encountered
				return 1;
			if ((c == '>') || (c == ' '))   // found a valid delimiter
				break;
			bufptr->append(QChar(c));
		} while ((c != '>') && (c != ' '));
		if (*bufptr == "/trk") // End of GPS track found: return EOF
			return 1;
		if (c == skipdelim)
			continue;  // if inappropriate delimiter was found, redo from start
		if (*bufptr == target)       // Compare xml element name from gpx file with the
			match = true;    // the target element searched for.
	} while (match == false);
	return 0;
}

// Find the coordinates at the time specified in coords.start_dive
// by searching the gpx file "fileName". Here is a typical trkpt element in GPX:
// <trkpt lat="-26.84" lon="32.88"><ele>-53.7</ele><time>2017-08-06T04:56:42Z</time></trkpt>
int getCoordsFromGPXFile(struct dive_coords *coords, QString fileName)
{
	struct tm tm1;
	time_t when = 0;
	double lon, lat;
	int line = 0;
	int64_t time_offset = coords->settingsDiff_offset + coords->timeZone_offset;
	time_t divetime;
	bool first_line = true;
	bool found = false;
	divetime = coords->start_dive;
	QString buf;
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

	do {
		line++;  // this is the sequence number of the trkpt xml element processed
		// Find next trkpt xml element (This function also detects </trk> that signals EOF):
		if (findXmlElement(&gpxFile, QString("trkpt"), &buf, ' ')) // This is the normal exit point
			break;                                        //    for this routine
		// == Get coordinates: ==
		if (getSubstring(&gpxFile, &buf, '"'))  // read up to the end of the "lat=" label
			break; // on EOF
		if (buf != "lat=") {
			fprintf(stderr, "GPX parse error: cannot find latitude (trkpt #%d)\n", line);
			return 1;
		}
		if (getSubstring(&gpxFile, &buf, '"'))  // read up to the end of the latitude value
			break; // on EOF
		lat = buf.toDouble();              // Convert lat to decimal
		if (getSubstring(&gpxFile, &buf, ' '))  // Read past space char
			break; // on EOF
		if (getSubstring(&gpxFile, &buf, '"'))  // Read up to end of "lon=" label
			break; // on EOF
		if (buf != "lon=") {
			fprintf(stderr, "GPX parse error: cannot find longitude (trkpt #%d)\n", line);
			return 1;
		}
		if (getSubstring(&gpxFile, &buf, '"'))  // get string with longitude
			break; // on EOF
		lon = buf.toDouble();              // Convert longitude to decimal
		// == get time: ==
		if (findXmlElement(&gpxFile, QString("time"), &buf, '>')) // Find the <time> element
			break; // on EOF
		if (getSubstring(&gpxFile, &buf, '<'))  // Read the string containing date/time
			break; // on EOF
		bool ok;
		tm1.tm_year = buf.left(4).toInt(&ok, 10);  // Extract the different substrings:
		tm1.tm_mon  = buf.mid(5,2).toInt(&ok,10) - 1;
		tm1.tm_mday = buf.mid(8,2).toInt(&ok,10);
		tm1.tm_hour = buf.mid(11,2).toInt(&ok,10);
		tm1.tm_min  = buf.mid(14,2).toInt(&ok,10);
		tm1.tm_sec  = buf.mid(17,2).toInt(&ok,10);
		when = utc_mktime(&tm1) + time_offset;
		if (first_line) {
			first_line = false;
			coords->start_track = when;   // Local time of start of GPS track
		}
		if ((when > divetime && found == false)) {   // This GPS local time corresponds to the start time of the dive
			coords->lon = lon; // save the coordinates
			coords->lat = lat;
			found = true;
		}
#ifdef GPSDEBUG
		utc_mkdate(when, &time); // print time and coordinates of each of the trkpt elements of the GPX file
		fprintf(stderr, " %02d: lat=%f lon=%f timestamp=%ld (%ld) %02d/%02d/%02d %02d:%02d  dt=%ld  %02d/%02d/%02d %02d:%02d\n", line, lat,
		lon, when, time_offset, time.tm_year, time.tm_mon+1, time.tm_mday, time.tm_hour, time.tm_min, divetime, dyr, dmon+1, dday,dhr, dmin );
#endif
	} while (true); // This loop executes until EOF causes a break out of the loop
	coords->end_track = when;  // This is the local time of the end of the GPS track
	gpxFile.close();
	return 0;
}
