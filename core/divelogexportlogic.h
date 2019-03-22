// SPDX-License-Identifier: GPL-2.0
#ifndef DIVELOGEXPORTLOGIC_H
#define DIVELOGEXPORTLOGIC_H

struct htmlExportSetting {
	bool exportPhotos;
	bool selectedOnly;
	bool listOnly;
	QString fontFamily;
	QString fontSize;
	int themeSelection;
	bool subsurfaceNumbers;
	bool yearlyStatistics;
	QString themeFile;
};

void exportHtmlInitLogic(const QString &filename, struct htmlExportSetting &hes);

#endif // DIVELOGEXPORTLOGIC_H

