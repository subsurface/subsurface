#include <QDebug>
#include <QPainter>
#include <QDesktopWidget>
#include <QApplication>
#include <QTextDocument>
#include <QAbstractTextDocumentLayout>
#include "mainwindow.h"
#include "printlayout.h"

/*
struct options {
	enum { PRETTY, TABLE, TWOPERPAGE } type;
	int print_selected;
	int color_selected;
	bool notes_up;
	int profile_height, notes_height, tanks_height;
};
*/

PrintLayout::PrintLayout(PrintDialog *dialogPtr, QPrinter *printerPtr, struct options *optionsPtr)
{
	dialog = dialogPtr;
	printer = printerPtr;
	printOptions = optionsPtr;
	// painter = new QPainter(printer);
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
	screenDpiY = desktop->physicalDpiX();

	printerDpi = printer->resolution();
	pageRect = printer->pageRect();

	scaleX = (qreal)printerDpi/(qreal)screenDpiX;
	scaleY = (qreal)printerDpi/(qreal)screenDpiY;
}

void PrintLayout::printSixDives()
{
	// nop
}

void PrintLayout::printTwoDives()
{
	// nop
}

void PrintLayout::printTable()
{
	QTextDocument doc;
	QSizeF pageSize;
    pageSize.setWidth(pageRect.width());
    pageSize.setHeight(pageRect.height());
    doc.setPageSize(pageSize);

	QString styleSheet = "<style type='text/css'>" \
		"table { border-width: 1px; border-style: solid; border-color: black; }" \
		"th { font-weight: bold; font-size: large; padding: 3px 10px 3px 10px; }" \
		"td { padding: 3px 10px 3px 10px; }" \
		"</style>";
	// setDefaultStyleSheet() doesn't work here?
	QString htmlText = styleSheet + "<table cellspacing='0' width='100%'>";
	QString htmlTextPrev;
	int pageCountNew = 1, pageCount = 1;
	bool insertHeading = true;

	while (pageCount < 3) { // should go trough dives (or selected) instead
		if (insertHeading) {
			htmlText += insertTableHeadingRow();
			insertHeading = false;
		}
		doc.setHtml(htmlText);
		pageCount = doc.pageCount();
		htmlTextPrev = htmlText;
		htmlText += insertTableDataRow();
		doc.setHtml(htmlText);
		pageCountNew = doc.pageCount();
		// if the page count increases here we revert and add a heading instead
		if (pageCountNew > pageCount) {
			htmlText = htmlTextPrev;
			doc.setHtml(htmlText);
			insertHeading = true;
		}
	}
	htmlText += "</table>";
	doc.setHtml(htmlText);
	doc.print(printer);
}

QString PrintLayout::insertTableHeadingRow()
{
	return "<tr><th>TITLE</th><th>TITLE 2</th></tr>";
}

QString PrintLayout::insertTableDataRow()
{
	return "<tr><td>hello</td></tr><tr><td>hello</td></tr>";
}
