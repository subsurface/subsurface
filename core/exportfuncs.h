// SPDX-License-Identifier: GPL-2.0
#ifndef EXPORTFUNCS_H
#define EXPORTFUNCS_H

#include <QObject>
#include <QFuture>
#include "core/dive.h"

class exportFuncs: public QObject {
	Q_OBJECT

public:
	static exportFuncs *instance();

	void exportProfile(QString filename, const bool selected_only);
	void export_TeX(const char *filename, const bool selected_only, bool plain);
	void export_depths(const char *filename, const bool selected_only);
	std::vector<const dive_site *> getDiveSitesToExport(bool selectedOnly);
	void exportUsingStyleSheet(QString filename, bool doExport, int units,
		QString stylesheet, bool anonymize);
	QFuture<int> future;

	// prepareDivesForUploadDiveLog
	// prepareDivesForUploadDiveShare

private:
	exportFuncs() {}

	// WARNING
	// saveProfile uses the UI and are therefore different between
	// Desktop (UI) and Mobile (QML)
	// In order to solve this difference, the actual implementations
	// are done in
	// desktop-widgets/divelogexportdialog.cpp and
	// mobile-widgets/qmlmanager.cpp
	void saveProfile(const struct dive *dive, const QString filename);
};

#endif // EXPORT_FUNCS_H
