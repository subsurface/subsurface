// SPDX-License-Identifier: GPL-2.0
#ifndef EXPORTFUNCS_H
#define EXPORTFUNCS_H

#include <QString>
#include <QFuture>

struct dive_site;

// A synchrounous callback interface to signal progress / check for user abort
struct ExportCallback {
	virtual void setProgress(int progress);	// 0-1000
	virtual bool canceled() const;
};

void exportProfile(QString filename, bool selected_only, ExportCallback &cb);
void export_TeX(const char *filename, bool selected_only, bool plain, ExportCallback &cb);
void export_depths(const char *filename, bool selected_only);
std::vector<const dive_site *> getDiveSitesToExport(bool selectedOnly);
QFuture<int> exportUsingStyleSheet(const QString &filename, bool doExport, int units, const QString &stylesheet, bool anonymize);

// prepareDivesForUploadDiveLog
// prepareDivesForUploadDiveShare

#endif // EXPORT_FUNCS_H
