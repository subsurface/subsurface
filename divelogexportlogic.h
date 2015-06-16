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

void file_copy_and_overwrite(const QString &fileName, const QString &newName);
void exportHtmlInitLogic(const QString &filename, struct htmlExportSetting &hes);

#endif // DIVELOGEXPORTLOGIC_H

