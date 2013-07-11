#ifndef PRINTLAYOUT_H
#define PRINTLAYOUT_H

#include <QPrinter>
#include <QStringList>

class PrintDialog;

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

	QStringList tableColumnNames;
	QStringList tableColumnWidths;

	void setup();
	void printSixDives() const;
	void printTwoDives() const;
	void printTable() const;
	QString insertTableHeadingRow() const;
	QString insertTableHeadingCol(int) const;
	QString insertTableDataRow(struct dive *) const;
	QString insertTableDataCol(QString) const;
};

#endif
