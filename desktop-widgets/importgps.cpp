// SPDX-License-Identifier: GPL-2.0
#include "desktop-widgets/importgps.h"
#include "core/subsurface-time.h"

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
	connect(ui.timeDiffEdit, &QTimeEdit::timeChanged, this, &ImportGPS::timeDiffEditChanged);
	connect(ui.timeZoneEdit, &QTimeEdit::timeChanged, this, &ImportGPS::timeZoneEditChanged);
	connect(ui.timezone_backwards, &QRadioButton::toggled, this, &ImportGPS::changeZoneBackwards);
	connect(ui.timezone_forward, &QRadioButton::toggled, this, &ImportGPS::changeZoneForward);
	connect(ui.diff_backwards, &QRadioButton::toggled, this, &ImportGPS::changeDiffBackwards);
	connect(ui.diff_forward, &QRadioButton::toggled, this, &ImportGPS::changeDiffForward);
	connect(ui.GPSbuttonBox, &QDialogButtonBox::clicked, this, &ImportGPS::buttonClicked);
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
	ui.startTimeLabel->setText(QString::number(time.tm_hour) + ":" + QString("%1").arg(time.tm_min, 2, 10, QChar('0'))); // track start time
	utc_mkdate(coords.end_track, &time);
	ui.endTimeLabel->setText(QString::number(time.tm_hour) + ":" + QString("%1").arg(time.tm_min, 2, 10, QChar('0')));  // track end time

	utc_mkdate(coords.start_dive, &time); // Display dive date and start and end times of dive:
	dive_day = time.tm_mday;
	datestr[0] = 0x0;
	strftime(datestr, sizeof(datestr), "%A %d %B ", localtime(&(coords.start_dive))); // dive date
	ui.diveDateLabel->setText("Dive date = " + QString(datestr) + QString::number(time.tm_year));
	ui.startTimeSyncLabel->setText( QString::number(time.tm_hour) + ":" + QString("%1").arg(time.tm_min, 2, 10, QChar('0'))); // dive start time
	utc_mkdate(coords.end_dive, &time);
	ui.endTimeSyncLabel->setText(QString::number(time.tm_hour) + ":" + QString("%1").arg(time.tm_min, 2, 10, QChar('0')));  // dive end time

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
	getCoordsFromGPXFile(&coords, fileName); // If any of the time controls are changed
	updateUI();          // .. then recalculate the synchronisation
}

void ImportGPS::changeZoneBackwards()
{
	if (coords.timeZone_offset > 0)
		coords.timeZone_offset = 0 - coords.timeZone_offset;
	getCoordsFromGPXFile(&coords, fileName);
	updateUI();
}

void ImportGPS::changeDiffForward()
{
	coords.settingsDiff_offset = abs(coords.settingsDiff_offset);
	getCoordsFromGPXFile(&coords, fileName);
	updateUI();
}

void ImportGPS::changeDiffBackwards()
{
	if (coords.settingsDiff_offset > 0)
		coords.settingsDiff_offset = 0 - coords.settingsDiff_offset;
	getCoordsFromGPXFile(&coords, fileName);
	updateUI();
}

void  ImportGPS::timeDiffEditChanged()
{
	coords.settingsDiff_offset = ui.timeDiffEdit->time().hour() * 3600 + ui.timeDiffEdit->time().minute() * 60;
	if (ui.diff_backwards->isChecked())
		coords.settingsDiff_offset = 0 - coords.settingsDiff_offset;
	getCoordsFromGPXFile(&coords, fileName);
	updateUI();
}

void  ImportGPS::timeZoneEditChanged()
{
	coords.timeZone_offset = ui.timeZoneEdit->time().hour() * 3600;
	if (ui.timezone_backwards->isChecked())
		coords.timeZone_offset = 0 - coords.timeZone_offset;
	getCoordsFromGPXFile(&coords, fileName);
	updateUI();
}
