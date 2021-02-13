// SPDX-License-Identifier: GPL-2.0
#ifndef PRINTER_H
#define PRINTER_H

#include "printoptions.h"
#include "templateedit.h"

class ProfileWidget2;
class QPainter;
class QPaintDevice;
class QRect;
class QWebView;

class Printer : public QObject {
	Q_OBJECT

public:
	enum PrintMode {
		PRINT,
		PREVIEW
	};

private:
	QPaintDevice *paintDevice;
	QWebView *webView;
	const print_options &printOptions;
	const template_options &templateOptions;
	QSize pageSize;
	PrintMode printMode;
	bool inPlanner;
	int done;
	int dpi;
	void render(int Pages);
	void flowRender();
	void putProfileImage(const QRect &box, const QRect &viewPort, QPainter *painter,
			     struct dive *dive, ProfileWidget2 *profile);

private slots:
	void templateProgessUpdated(int value);

public:
	Printer(QPaintDevice *paintDevice, const print_options &printOptions, const template_options &templateOptions, PrintMode printMode, bool inPlanner);
	~Printer();
	void print();
	void previewOnePage();
	QString exportHtml();

signals:
	void progessUpdated(int value);
};

#endif //PRINTER_H
