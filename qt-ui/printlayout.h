#ifndef PRINTLAYOUT_H
#define PRINTLAYOUT_H

#include <QObject>
#include <QPrinter>
#include <QList>

class PrintDialog;
class TablePrintModel;
struct dive;

class PrintLayout : public QObject {
	Q_OBJECT

public:
	PrintLayout(PrintDialog *, QPrinter *, struct options *);
	void print();

private:
	PrintDialog *dialog;
	QPrinter *printer;
	QPainter *painter;
	struct options *printOptions;

	int screenDpiX, screenDpiY, printerDpi, scaledPageW, scaledPageH;
	qreal scaleX, scaleY;
	QRect pageRect;

	QList<QString> tablePrintColumnNames;
	QList<unsigned int> tablePrintColumnWidths;
	unsigned int tablePrintHeadingBackground;

	void setup();
	void printProfileDives(int divesPerRow, int divesPerColumn);
	void printTable();
	void addTablePrintDataRow(TablePrintModel *model, int row, struct dive *dive) const;
	void addTablePrintHeadingRow(TablePrintModel *model, int row) const;

	QPixmap convertPixmapToGrayscale(QPixmap) const;
};

#endif
