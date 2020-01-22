// SPDX-License-Identifier: GPL-2.0
#ifndef IMPORT_GPS_H
#define IMPORT_GPS_H

#include "ui_importgps.h"
#include "desktop-widgets/locationinformation.h"
#include "core/parse-gpx.h"

#include <QFile>

class ImportGPS : public QDialog {
	Q_OBJECT
public:
	Ui::ImportGPS ui;
	explicit ImportGPS(QWidget *parent, QString fileName, class Ui::LocationInformation *LocationUI);
	struct dive_coords coords;
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
	int pixmapSize;
};

#endif

