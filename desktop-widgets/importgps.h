// SPDX-License-Identifier: GPL-2.0
#ifndef IMPORT_GPS_H
#define IMPORT_GPS_H

#include "ui_importgps.h"
#include "desktop-widgets/locationinformation.h"

#include <QFile>

struct dive_coords {         // This structure holds important information after parsing the GPX file:
	time_t start_dive;            // Start time of the current dive, obtained using current_dive (local time)
	time_t end_dive;              // End time of current dive (local time)
	time_t start_track;           // Start time of GPX track (UCT)
	time_t end_track;             // End time of GPX track (UCT)
	double lon;                   // Longitude of the first trackpoint after the start of the dive
	double lat;                   // Latitude of the first trackpoint after the start of the dive
	int64_t settingsDiff_offset;  // Local time difference between dive computer and GPS equipment
	int64_t timeZone_offset;      // UCT international time zone offset of dive site
};

class ImportGPS : public QDialog {
	Q_OBJECT
public:
	Ui::ImportGPS ui;
	explicit ImportGPS(QWidget *parent, QString fileName, class Ui::LocationInformation *LocationUI);
	struct dive_coords coords;
	int getCoordsFromFile();
	void updateUI();

private
slots:
	void timeDiffEditChanged();
	void timeZoneEditChanged();
	void changeZoneForward();
	void changeZoneBackwards();
	void changeDiffForward();
	void changeDiffBackwards();
	void buttonClicked(QAbstractButton *button);

private:
	QString fileName;
	class Ui::LocationInformation *LocationUI;
	int getSubstring(QFile *f, QString *buf, char delim);
	int findXmlElement(QFile *f, QString target, QString *buf, char termc);
	int pixmapSize;
};

#endif

