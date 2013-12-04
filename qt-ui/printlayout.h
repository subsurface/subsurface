#ifndef PRINTLAYOUT_H
#define PRINTLAYOUT_H

#include <QObject>
#include <QPrinter>
#include <QList>

class QTableView;
class PrintDialog;
class TablePrintModel;
class ProfilePrintModel;
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

	int screenDpiX, screenDpiY, printerDpi, scaledPageW, scaledPageH;
	qreal scaleX, scaleY;
	QRect pageRect;

	QList<QString> tablePrintColumnNames;
	unsigned int tablePrintHeadingBackground;
	QList<unsigned int> tablePrintColumnWidths;
	unsigned int profilePrintTableMaxH;
	QList<unsigned int> profilePrintColumnWidths, profilePrintRowHeights;

	void setup();
	void estimateTotalDives(struct dive *dive, int *i, int *total) const;
	void printProfileDives(int divesPerRow, int divesPerColumn);
	QTableView *createProfileTable(ProfilePrintModel *model, const int tableW);
	void printTable();
	void addTablePrintDataRow(TablePrintModel *model, int row, struct dive *dive) const;
	void addTablePrintHeadingRow(TablePrintModel *model, int row) const;

signals:
    void signalProgress(int);
};

#endif
