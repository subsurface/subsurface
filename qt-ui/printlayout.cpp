#include <QDebug>
#include <QPainter>
#include <QDesktopWidget>
#include <QApplication>
#include <QTextDocument>
#include <QAbstractTextDocumentLayout>
#include "mainwindow.h"
#include "printlayout.h"
#include "../dive.h"
#include "../display.h"
#include "models.h"

/*
struct options {
	enum { PRETTY, TABLE, TWOPERPAGE } type;
	int print_selected;
	int color_selected;
	bool notes_up;
	int profile_height, notes_height, tanks_height;
};
*/

#define TABLE_PRINT_COL 7

PrintLayout::PrintLayout(PrintDialog *dialogPtr, QPrinter *printerPtr, struct options *optionsPtr)
{
	dialog = dialogPtr;
	printer = printerPtr;
	printOptions = optionsPtr;
	// painter = new QPainter(printer);

	// table print settings
	tableColumnNames.append(tr("Dive#"));
	tableColumnNames.append(tr("Date"));
	tableColumnNames.append(tr("Depth"));
	tableColumnNames.append(tr("Duration"));
	tableColumnNames.append(tr("Master"));
	tableColumnNames.append(tr("Buddy"));
	tableColumnNames.append(tr("Location"));
	tableColumnWidths.append("7");
	tableColumnWidths.append("10");
	tableColumnWidths.append("10");
	tableColumnWidths.append("10");
	tableColumnWidths.append("15");
	tableColumnWidths.append("15");
	tableColumnWidths.append("100");
}

void PrintLayout::print()
{
	// we call setup each time to check if the printer properties have changed
	setup();

	// temp / debug
	printTable();
	return;
	// ------------
	switch (printOptions->type) {
	case options::PRETTY:
		printSixDives();
		break;
	case options::TWOPERPAGE:
		printTwoDives();
		break;
	case options::TABLE:
		printTable();
		break;
	}
}

void PrintLayout::setup()
{
	QDesktopWidget *desktop = QApplication::desktop();
	screenDpiX = desktop->physicalDpiX();
	screenDpiY = desktop->physicalDpiY();

	printerDpi = printer->resolution();
	pageRect = printer->pageRect();

	scaleX = (qreal)printerDpi/(qreal)screenDpiX;
	scaleY = (qreal)printerDpi/(qreal)screenDpiY;
}

void PrintLayout::printSixDives() const
{
	// nop
}

void PrintLayout::printTwoDives() const
{
	// nop
}

void PrintLayout::printTable() const
{
	QTextDocument doc;
	QSizeF pageSize;
	pageSize.setWidth(pageRect.width());
	pageSize.setHeight(pageRect.height());
	doc.documentLayout()->setPaintDevice(printer);
	doc.setPageSize(pageSize);

	QString styleSheet(
		"<style type='text/css'>"
		"table {"
		"	border-width: 1px;"
		"	border-style: solid;"
		"	border-color: #999999;"
		"}"
		"th {"
		"	background-color: #eeeeee;"
		"	font-size: small;"
		"	padding: 3px 5px 3px 5px;"
		"}"
		"td {"
		"	font-size: small;"
		"	padding: 3px 5px 3px 5px;"
		"}" 
		"</style>"
	);
	// setDefaultStyleSheet() doesn't work here?
	QString htmlText = styleSheet + "<table cellspacing='0' width='100%'>";
	QString htmlTextPrev;
	int pageCountNew = 1, pageCount;
	bool insertHeading = true;

	int i;
	struct dive *dive;
	for_each_dive(i, dive) {
		if (!dive->selected && printOptions->print_selected)
			continue;
		if (insertHeading) {
			htmlText += insertTableHeadingRow();
			insertHeading = false;
		}
		htmlTextPrev = htmlText;
		htmlText += insertTableDataRow(dive);
		doc.setHtml(htmlText);
		pageCount = pageCountNew;
		pageCountNew = doc.pageCount();
		/* if the page count increases after adding this row we 'revert'
		 * and add a heading instead. */
		if (pageCountNew > pageCount) {
			htmlText = htmlTextPrev;
			insertHeading = true;
			i--;
		}
	}
	htmlText += "</table>";
	doc.setHtml(htmlText);
	doc.print(printer);
}

QString PrintLayout::insertTableHeadingRow() const
{
	int i;
	QString ret("<tr>");
	for (i = 0; i < TABLE_PRINT_COL; i++)
		ret += insertTableHeadingCol(i);
	ret += "</tr>";
	return ret;
}

QString PrintLayout::insertTableHeadingCol(int col) const
{
	QString ret("<th align='left' width='");
	ret += tableColumnWidths.at(col);
	ret += "%'>";
	ret += tableColumnNames.at(col);
	ret += "</th>";
	return ret;
}

QString PrintLayout::insertTableDataRow(struct dive *dive) const
{
	// use the DiveItem class
	struct DiveItem di;
	di.dive = dive;

	// fill row
	QString ret("<tr>");
	ret += insertTableDataCol(QString::number(dive->number));
	ret += insertTableDataCol(di.displayDate());
	ret += insertTableDataCol(di.displayDepth());
	ret += insertTableDataCol(di.displayDuration());
	ret += insertTableDataCol(dive->divemaster);
	ret += insertTableDataCol(dive->buddy);
	ret += insertTableDataCol(dive->location);
	ret += "</tr>";
	return ret;
}

QString PrintLayout::insertTableDataCol(QString data) const
{
	return "<td>" + data + "</td>";
}
