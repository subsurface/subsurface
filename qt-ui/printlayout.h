#ifndef PRINTLAYOUT_H
#define PRINTLAYOUT_H

#include <QPrinter>
#include <QPainter>
#include "../display.h"

class PrintDialog;
struct dive;

class PrintLayout : public QObject {
	Q_OBJECT

public:
	PrintLayout(PrintDialog *, QPrinter *, struct options *);
	void print();

private:
	PrintDialog *dialog;
	QPrinter *printer;
	struct options *printOptions;

	QPainter *painter;
	int screenDpiX, screenDpiY, printerDpi;
	qreal scaleX, scaleY;
	QRect pageRect;

	void setup();
	void printSixDives();
	void printTwoDives();
	void printTable();
	QString insertTableHeadingRow();
	QString insertTableDataRow(struct dive *dive);
};

#endif
