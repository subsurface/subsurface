// SPDX-License-Identifier: GPL-2.0
#include "ui_importgps.h"
#include "desktop-widgets/importgps.h"
#include "desktop-widgets/locationinformation.h"
#include "core/dive.h"

#include <QFileDialog>
#include <QProcess>
#include <QDateTime>
#include <QDate>

/* Import dive coordinates from a GPS device and synchronise them with the dive profile information
   of a dive computer. This file contains the infrastructure to:
   1) Read a .GPX file from a GPS system.
   2) Find the first gpx trackpoint that follows after the start of the dive.
   3) Allow modification of the coordinates dependent on international time zone and
              on differences in local time settings between the GPS and the dive computer.
   4) Saving the coordinates into the Coordinates text box in the Dive Site Edit panel
              and which which causes the map to show the location of the dive site. 
   The structure coords is used to store critical information.  */
ImportGPS::ImportGPS(QWidget *parent, QString fileName, class Ui::LocationInformation *LocationUI) : QDialog(parent),
	fileName(fileName), LocationUI(LocationUI)
{
	ui.setupUi(this);
	connect(ui.timeDiffEdit, SIGNAL(timeChanged(const QTime)), this, SLOT(timeDiffEditChanged()));
	connect(ui.timeZoneEdit, SIGNAL(timeChanged(const QTime)), this, SLOT(timeZoneEditChanged()));
	connect(ui.timezone_backwards, SIGNAL(toggled(bool)), this, SLOT(changeZoneBackwards()));
	connect(ui.timezone_forward, SIGNAL(toggled(bool)), this, SLOT(changeZoneForward()));
	connect(ui.diff_backwards, SIGNAL(toggled(bool)), this, SLOT(changeDiffBackwards()));
	connect(ui.diff_forward, SIGNAL(toggled(bool)), this, SLOT(changeDiffForward()));
	connect(ui.GPSbuttonBox, SIGNAL(clicked(QAbstractButton *)), this, SLOT(buttonClicked(QAbstractButton *)));
	coords.settingsDiff_offset = 0;
	coords.timeZone_offset = 0;
	coords.lat = 0;
	coords.lon = 0;
	pixmapSize = (int) (ui.diveDateLabel->height() / 2);
}

void ImportGPS::buttonClicked(QAbstractButton *button)
{
	if (ui.GPSbuttonBox->buttonRole(button) == QDialogButtonBox::AcceptRole) {
		// Write the coordinates in decimal degree format to the Coordinates QLineEdit of the LocationaInformationWidget UI:
		LocationUI->diveSiteCoordinates->setText(QString::number(coords.lat) + ", " + QString::number(coords.lon));
		LocationUI->diveSiteCoordinates->editingFinished();
	} else {
		close();
	}
}

//  Read text from the present position in the file until
//  the first 'delim' character is encountered.
int ImportGPS::getSubstring(QFile *file, QString *bufptr, char delim)
{
	char c;
	bufptr->clear();
	do {
		if (file->read(&c, 1) <= 0) // EOF
			return 1;
		if ( c == delim) break;
		bufptr->append(QChar(c));
	} while (c != delim);
	return 0;
}

// Find the next occurence of a specified target GPX element in the file,
// characerised by a "<xxx " or "<xxx>" character sequence.
// 'target' specifies the name of the element searched for.
// termc is the ending character of the element name search = '>' or ' '.
int ImportGPS::findXmlElement(QFile *fileptr, QString target, QString *bufptr, char termc)
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
			continue;  // if wrong delimiter for this element was found, redo from start
		if (*bufptr == target)       // Compare xml element name from gpx file with the
			match = true;    // the target element searched for.
	} while (match == false);
	return 0;
}

// Find the coordinates at the time specified in coords.start_dive
// by searching the gpx file "fileName". Here is a typical trkpt element in GPX:
// <trkpt lat="-26.84" lon="32.88"><ele>-53.7</ele><time>2017-08-06T04:56:42Z</time></trkpt>
int ImportGPS::getCoordsFromFile() 
{
	struct tm tm1;
	time_t when = 0;
	double lon, lat;
	int line = 0;
	int64_t time_offset = coords.settingsDiff_offset + coords.timeZone_offset;
	time_t divetime;
	bool first_line = true;
	bool found = false;
	divetime = coords.start_dive;
	QString buf;
	QFile f1;
	f1.setFileName(fileName);
	if (!f1.open(QIODevice::ReadOnly | QIODevice::Text)) {
		QByteArray local8bitBAString1 = fileName.toLocal8Bit();
		char *fname = local8bitBAString1.data();   // convert QString to a C string filename
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
		if (findXmlElement(&f1, QString("trkpt"), &buf, ' ')) // This is the normal exit point
			break;                                        //    for this routine
		// == Get coordinates: ==
		if (getSubstring(&f1, &buf, '"'))  // read up to the end of the "lat=" label
			break; // on EOF
		if (buf != "lat=") {
			fprintf(stderr, "GPX parse error: cannot find latitude (trkpt #%d)\n", line);
			return 1;
		}
		if (getSubstring(&f1, &buf, '"'))  // read up to the end of the latitude value
			break; // on EOF
		lat = buf.toDouble();                  // Convert lat to decimal
		if (getSubstring(&f1, &buf, ' '))  // Read past space char
			break; // on EOF
		if (getSubstring(&f1, &buf, '"'))  // Read up to end of "lon=" label
			break; // on EOF
		if (buf != "lon=") {
			fprintf(stderr, "GPX parse error: cannot find longitude (trkpt #%d)\n", line);
			return 1;
		}
		if (getSubstring(&f1, &buf, '"'))  // get string with longitude
			break; // on EOF
		lon = buf.toDouble();                  // Convert longitude to decimal
		// == get time: ==
		if (findXmlElement(&f1, QString("time"), &buf, '>')) // Find the <time> element
			break; // on EOF
		if (getSubstring(&f1, &buf, '<'))  // Read the string containing date/time
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
			coords.start_track = when;   // Local time of start of GPS track
		}
		if ((when > divetime) && (found == false)) {   // This GPS local time corresponds to the start time of the dive
			coords.lon = lon; // save the coordinates
			coords.lat = lat;
			found = true;
		}
#ifdef GPSDEBUG
		utc_mkdate(when, &time); // print time and coordinates of each of the trkpt elements of the GPX file
		fprintf(stderr, " %02d: lat=%f lon=%f timestamp=%ld (%ld) %02d/%02d/%02d %02d:%02d  dt=%ld  %02d/%02d/%02d %02d:%02d\n", line, lat,
		lon, when, time_offset, time.tm_year, time.tm_mon+1, time.tm_mday, time.tm_hour, time.tm_min, divetime, dyr, dmon+1, dday,dhr, dmin );
#endif
	} while (true); // This loop executes until EOF causes a break out of the loop
	coords.end_track = when;  // This is the local time of the end of the GPS track
	f1.close();
	return 0;
}

// Fill the visual elements of the synchronisation panel with information
void  ImportGPS::updateUI()
{
	struct tm time;
	int dive_day, gps_day;
	char datestr[50];
	QString problemString = "";

	utc_mkdate(coords.start_track, &time); // Display GPS date and local start and end times of track:
	gps_day = time.tm_mday;
	datestr[0] = 0x0;
	strftime(datestr, sizeof(datestr), "%A %d %B ", &time); // GPS date
	ui.trackDateLabel->setText("GPS date  = " + QString(datestr) + QString::number(time.tm_year));
	ui.startTimeLabel->setText(QString::number(time.tm_hour) + ":" + QString::number(time.tm_min)); // track start time
	utc_mkdate(coords.end_track, &time);
	ui.endTimeLabel->setText(QString::number(time.tm_hour) + ":" + QString::number(time.tm_min));  // track end time

	utc_mkdate(coords.start_dive, &time); // Display dive date and start and end times of dive:
	dive_day = time.tm_mday;
	datestr[0] = 0x0;
	strftime(datestr, sizeof(datestr), "%A %d %B ", localtime(&(coords.start_dive))); // dive date
	ui.diveDateLabel->setText("Dive date = " + QString(datestr) + QString::number(time.tm_year));
	ui.startTimeSyncLabel->setText( QString::number(time.tm_hour) + ":" + QString::number(time.tm_min)); // dive start time
	utc_mkdate(coords.end_dive, &time);
	ui.endTimeSyncLabel->setText(QString::number(time.tm_hour) + ":" + QString::number(time.tm_min));  // dive end time

	// This section implements extensive warnings in case there is not synchronisation between dive and GPS data:

	if (gps_day != dive_day)
		problemString = "(different dates)";
	// Create 3 icons to indicate the quality of the synchrinisation between dive and GPS
	QPixmap goodResultIcon (":gps_good_result-icon");
	ui.goodIconLabel->setPixmap(goodResultIcon.scaled(pixmapSize,pixmapSize,Qt::KeepAspectRatio));
	ui.goodIconLabel->setVisible(false);

	QPixmap warningResultIcon (":gps_warning_result-icon");
	ui.warningIconLabel->setPixmap(warningResultIcon.scaled(pixmapSize,pixmapSize,Qt::KeepAspectRatio));
	ui.warningIconLabel->setVisible(false);

	QPixmap badResultIcon (":gps_bad_result-icon");
	ui.badIconLabel->setPixmap(badResultIcon.scaled(pixmapSize,pixmapSize,Qt::KeepAspectRatio));
	ui.badIconLabel->setVisible(false);
	// Show information or warning message as well as synch quality icon
	if (coords.start_dive < coords.start_track) {
		ui.resultLabel->setStyleSheet("QLabel { color: red;} ");
		ui.resultLabel->setText("PROBLEM: Dive started before the GPS track "+ problemString);
		ui.badIconLabel->setVisible(true);
	} else {
		if (coords.start_dive > coords.end_track) {
			ui.resultLabel->setStyleSheet("QLabel { color: red;} ");
			ui.resultLabel->setText("PROBLEM: Dive started after the GPS track " + problemString);
			ui.badIconLabel->setVisible(true);
		} else {
			if (coords.end_dive > coords.end_track) {
				ui.resultLabel->setStyleSheet("QLabel { color: red;} ");
				ui.resultLabel->setText("WARNING: Dive ended after the GPS track " + problemString);
				ui.warningIconLabel->setVisible(true);
			} else {
				ui.resultLabel->setStyleSheet("QLabel { color: darkgreen;} ");
				ui.resultLabel->setText("Dive coordinates: "+ QString::number(coords.lat) + "S, " + QString::number(coords.lon) + "E");
				ui.goodIconLabel->setVisible(true);
			}
		}
	}
}

void ImportGPS::changeZoneForward()
{
	coords.timeZone_offset = abs(coords.timeZone_offset);
	getCoordsFromFile(); // If any of the time controls are changed
	updateUI();          // .. then recalculate the synchronisation
}

void ImportGPS::changeZoneBackwards()
{
	if (coords.timeZone_offset > 0)
		coords.timeZone_offset = 0 - coords.timeZone_offset;
	getCoordsFromFile();
	updateUI();
}

void ImportGPS::changeDiffForward()
{
	coords.settingsDiff_offset = abs(coords.settingsDiff_offset);
	getCoordsFromFile();
	updateUI();
}

void ImportGPS::changeDiffBackwards()
{
	if (coords.settingsDiff_offset > 0)
		coords.settingsDiff_offset = 0 - coords.settingsDiff_offset;
	getCoordsFromFile();
	updateUI();
}

void  ImportGPS::timeDiffEditChanged()
{
	coords.settingsDiff_offset = ui.timeDiffEdit->time().hour() * 3600 + ui.timeDiffEdit->time().minute() * 60;
	if (ui.diff_backwards->isChecked())
		coords.settingsDiff_offset = 0 - coords.settingsDiff_offset;
	getCoordsFromFile();
	updateUI();
}

void  ImportGPS::timeZoneEditChanged()
{
	coords.timeZone_offset = ui.timeZoneEdit->time().hour() * 3600;
	if (ui.timezone_backwards->isChecked())
		coords.timeZone_offset = 0 - coords.timeZone_offset;
	getCoordsFromFile();
	updateUI();
}

