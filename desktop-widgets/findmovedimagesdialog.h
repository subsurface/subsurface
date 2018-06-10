// SPDX-License-Identifier: GPL-2.0
#ifndef FINDMOVEDIMAGES_H
#define FINDMOVEDIMAGES_H

#include "ui_findmovedimagesdialog.h"
#include <QFutureWatcher>
#include <QVector>
#include <QMap>
#include <QAtomicInteger>

class FindMovedImagesDialog : public QDialog {
	Q_OBJECT
public:
	FindMovedImagesDialog(QWidget *parent = 0);
private
slots:
	void on_scanButton_clicked();
	void apply();
	void on_buttonBox_rejected();
	void setProgress(double progress, QString path);
	void searchDone();
private:
	struct Match {
		QString originalFilename;
		QString localFilename;
		int matchingPathItems;
	};
	struct ImageMatch {
		QString localFilename;
		int score;
	};
	struct ImagePath {
		QString fullPath;
		QString	filenameUpperCase;
		ImagePath() = default;		// For some reason QVector<...>::reserve() needs a default constructor!?
		ImagePath(const QString &path);
		inline bool operator<(const ImagePath &path2) const;
	};
	Ui::FindMovedImagesDialog ui;
	QFutureWatcher<QVector<Match>> watcher;
	QVector<Match> matches;
	QAtomicInt stopScanning;
	QScopedPointer<QFontMetrics> fontMetrics;		// Needed to format elided paths

	void learnImage(const QString &filename, QMap<QString, ImageMatch> &matches, const QVector<ImagePath> &imagePaths);
	QVector<Match> learnImages(const QString &dir, int maxRecursions, QVector<QString> imagePaths);
};

#endif
