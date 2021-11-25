// SPDX-License-Identifier: GPL-2.0
#ifndef PRINTER_H
#define PRINTER_H

#include "printoptions.h"

#include <QPrinter>
#include <QTemporaryDir>

class ProfileWidget2;
class QPainter;
class QPaintDevice;
class QRect;
class QWebEngineView;


class Printer : public QObject {
	Q_OBJECT

public:
	enum PrintMode {
		PRINT,
		PREVIEW
	};
	QWebEngineView *webView;

private:
	QPaintDevice *paintDevice;
	QPrinter printer;
	QTemporaryDir printDir;
	print_options &printOptions;
	template_options &templateOptions;
	QSize pageSize;
	PrintMode printMode;
	bool inPlanner;
	int done;
	void onLoadFinished();
	bool profiles_missing;

private slots:
	void templateProgessUpdated(int value);
	void printing();

public:
	Printer(QPaintDevice *paintDevice, print_options &printOptions, template_options &templateOptions, PrintMode printMode, bool inPlanner, QWidget *parent);
	~Printer();
	void print();
	void previewOnePage();
	QString writeTmpTemplate(const QString templtext);
	QString exportHtml();
	void updateOptions(print_options &poptions, template_options &toptions);


signals:
	void progessUpdated(int value);
	void profilesInserted();
	void jobDone();
};

#endif //PRINTER_H
