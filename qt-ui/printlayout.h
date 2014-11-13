#ifndef PRINTLAYOUT_H
#define PRINTLAYOUT_H

#include <QObject>
#include <QList>
#include <QVector>
#include <QRect>

class QPrinter;
class QTableView;
class PrintDialog;
class TablePrintModel;
class ProfilePrintModel;
struct dive;

class PrintLayout : public QObject {
	Q_OBJECT

public:
	PrintLayout(PrintDialog *, QPrinter *, struct print_options *);
	void print();

private:
	PrintDialog *dialog;
	QPrinter *printer;
	struct print_options *printOptions;

	int screenDpiX, screenDpiY, printerDpi, pageW, pageH;
	QRect pageRect;

	QVector<QString> tablePrintColumnNames;
	unsigned int tablePrintHeadingBackground;
	QList<unsigned int> tablePrintColumnWidths;
	unsigned int profilePrintTableMaxH;
	QList<unsigned int> profilePrintColumnWidths, profilePrintRowHeights;

	void setup();
	int estimateTotalDives() const;
	void printProfileDives(int divesPerRow, int divesPerColumn);
	QTableView *createProfileTable(ProfilePrintModel *model, const int tableW, const qreal fitNotesToHeight = 0.0);
	void printTable();
	void addTablePrintDataRow(TablePrintModel *model, int row, struct dive *dive) const;
	void addTablePrintHeadingRow(TablePrintModel *model, int row) const;

signals:
	void signalProgress(int);
};

#endif // PRINTLAYOUT_H
