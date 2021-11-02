// SPDX-License-Identifier: GPL-2.0
#ifndef PRINTER_H
#define PRINTER_H

#include "printoptions.h"
#include "templateedit.h"
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
	QWebEngineView *webView;
	enum PrintMode {
		PRINT,
		PREVIEW
	};

private:
	QPaintDevice *paintDevice;
	QPrinter printer;
	QTemporaryDir printDir;
	const print_options &printOptions;
	const template_options &templateOptions;
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
	Printer(QPaintDevice *paintDevice, const print_options &printOptions, const template_options &templateOptions, PrintMode printMode, bool inPlanner);
	~Printer();
	void print();
	void previewOnePage();
	QString exportHtml();

signals:
	void progessUpdated(int value);
	void profiles_inserted();
	void jobDone();
};

#endif //PRINTER_H
