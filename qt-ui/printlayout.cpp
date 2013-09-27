#include <QtCore/qmath.h>
#include <QDebug>
#include <QPainter>
#include <QDesktopWidget>
#include <QApplication>
#include <QTableView>
#include <QHeaderView>
#include "mainwindow.h"
#include "profilegraphics.h"
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

PrintLayout::PrintLayout(PrintDialog *dialogPtr, QPrinter *printerPtr, struct options *optionsPtr)
{
	dialog = dialogPtr;
	printer = printerPtr;
	painter = NULL;
	printOptions = optionsPtr;

	// table print settings
	tablePrintHeadingBackground = 0xffeeeeee;
	tablePrintColumnNames.append(tr("Dive#"));
	tablePrintColumnNames.append(tr("Date"));
	tablePrintColumnNames.append(tr("Depth"));
	tablePrintColumnNames.append(tr("Duration"));
	tablePrintColumnNames.append(tr("Master"));
	tablePrintColumnNames.append(tr("Buddy"));
	tablePrintColumnNames.append(tr("Location"));
	tablePrintColumnWidths.append(7);
	tablePrintColumnWidths.append(10);
	tablePrintColumnWidths.append(10);
	tablePrintColumnWidths.append(10);
	tablePrintColumnWidths.append(15);
	tablePrintColumnWidths.append(15);
	tablePrintColumnWidths.append(33);
}

void PrintLayout::print()
{
	// we call setup each time to check if the printer properties have changed
	setup();
	switch (printOptions->type) {
	case options::PRETTY:
		printProfileDives(3, 2);
		break;
	case options::TWOPERPAGE:
		printProfileDives(2, 1);
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

	// a printer page scalled to screen DPI
	scaledPageW = pageRect.width() / scaleX;
	scaledPageH = pageRect.height() / scaleY;
}

void PrintLayout::printProfileDives(int divesPerRow, int divesPerColumn)
{
	// setup a painter
	painter = new QPainter();
	painter->begin(printer);
	painter->setRenderHint(QPainter::Antialiasing);
	painter->setRenderHint(QPainter::SmoothPixmapTransform);
	painter->scale(scaleX, scaleY);

	// setup the profile widget
	ProfileGraphicsView *profile = mainWindow()->graphics();
	profile->clear();
	profile->setPrintMode(true, !printOptions->color_selected);
	QSize originalSize = profile->size();
	// swap rows/col for landscape
	if (printer->orientation() == QPrinter::Landscape) {
		int swap = divesPerColumn;
		divesPerColumn = divesPerRow;
		divesPerRow = swap;
	}
	// estimate profile and table height and resize the widget
	const int scaledW = scaledPageW / divesPerColumn;
	const int scaledH = scaledPageH / divesPerRow;
	/* make the table 1/3 of the reserved height. potentially this could depend
	 * on orientation and other parameters as well. */
	const int tableHeight = scaledH / 3;
	profile->resize(scaledW, scaledH - tableHeight);

	// plot the dives at specific rows and columns
	int i, row = 0, col = 0;
	struct dive *dive;
	for_each_dive(i, dive) {
		if (!dive->selected && printOptions->print_selected)
			continue;
		if (col == divesPerColumn) {
			col = 0;
			row++;
			if (row == divesPerRow) {
				row = 0;
				printer->newPage();
			}
		}
		profile->plot(dive, true);
		QPixmap pm = QPixmap::grabWidget(profile);
		painter->drawPixmap(scaledW * col, scaledH * row, pm);
		/* TODO: table should be drawn here, preferably by another function */
		col++;
	}

	// cleanup
	painter->end();
	delete painter;
	painter = NULL;
	profile->setPrintMode(false);
	profile->resize(originalSize);
	profile->clear();
	profile->plot(current_dive, true);
}

void PrintLayout::printTable()
{
	// create and setup a table
	QTableView table;
	table.setAttribute(Qt::WA_DontShowOnScreen);
	table.setSelectionMode(QAbstractItemView::NoSelection);
	table.setFocusPolicy(Qt::NoFocus);
	table.horizontalHeader()->setVisible(false);
	table.horizontalHeader()->setResizeMode(QHeaderView::Fixed);
	table.verticalHeader()->setVisible(false);
	table.verticalHeader()->setResizeMode(QHeaderView::ResizeToContents);
	table.setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	table.setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	// fit table to one page initially
	table.resize(scaledPageW,  scaledPageH);

	// create and fill a table model
	TablePrintModel model;
	struct dive *dive;
	int i, row = 0;
	addTablePrintHeadingRow(&model, row); // add one heading row
	row++;
	for_each_dive(i, dive) {
		if (!dive->selected && printOptions->print_selected)
			continue;
		addTablePrintDataRow(&model, row, dive);
		row++;
	}
	table.setModel(&model); // set model to table
	// resize columns to percentages from page width
	for (int i = 0; i < model.columns; i++) {
		int pw = qCeil((qreal)(tablePrintColumnWidths.at(i) * table.width()) / 100);
		table.horizontalHeader()->resizeSection(i, pw);
	}
	// reset the model at this point
	model.callReset();

	// a list of vertical offsets where pages begin and some helpers
	QList<unsigned int> pageIndexes;
	pageIndexes.append(0);
	int tableHeight = 0, rowH = 0, accH = 0;

	// process all rows
	for (int i = 0; i < model.rows; i++) {
		rowH = table.rowHeight(i);
		accH += rowH;
		if (accH > scaledPageH) { // push a new page index and add a heading
			pageIndexes.append(pageIndexes.last() + (accH - rowH));
			addTablePrintHeadingRow(&model, i);
			accH = 0;
			i--;
		}
		tableHeight += rowH;
	}
	pageIndexes.append(pageIndexes.last() + accH);
	// resize the whole widget so that it can be rendered
	table.resize(scaledPageW, tableHeight);

	// attach a painter and render pages by using pageIndexes
	QPainter painter(printer);
	painter.setRenderHint(QPainter::Antialiasing);
	painter.setRenderHint(QPainter::SmoothPixmapTransform);
	painter.scale(scaleX, scaleY);
	for (int i = 0; i < pageIndexes.size() - 1; i++) {
		if (i > 0)
			printer->newPage();
		QRegion region(0, pageIndexes.at(i) - 1,
		               table.width(),
		               pageIndexes.at(i + 1) - pageIndexes.at(i) + 2);
		table.render(&painter, QPoint(0, 0), region);
	}
}

void PrintLayout::addTablePrintDataRow(TablePrintModel *model, int row, struct dive *dive) const
{
	struct DiveItem di;
	di.dive = dive;
	model->insertRow();
	model->setData(model->index(row, 0), QString::number(dive->number), Qt::DisplayRole);
	model->setData(model->index(row, 1), di.displayDate(), Qt::DisplayRole);
	model->setData(model->index(row, 2), di.displayDepth(), Qt::DisplayRole);
	model->setData(model->index(row, 3), di.displayDuration(), Qt::DisplayRole);
	model->setData(model->index(row, 4), dive->divemaster, Qt::DisplayRole);
	model->setData(model->index(row, 5), dive->buddy, Qt::DisplayRole);
	model->setData(model->index(row, 6), dive->location, Qt::DisplayRole);
}

void PrintLayout::addTablePrintHeadingRow(TablePrintModel *model, int row) const
{
	model->insertRow(row);
	for (int i = 0; i < model->columns; i++) {
		model->setData(model->index(row, i), tablePrintColumnNames.at(i), Qt::DisplayRole);
		model->setData(model->index(row, i), tablePrintHeadingBackground, Qt::BackgroundRole);
	}
}

// experimental
QPixmap PrintLayout::convertPixmapToGrayscale(QPixmap pixmap) const
{
	QImage image = pixmap.toImage();
	int gray, width = pixmap.width(), height = pixmap.height();
	for (int i = 0; i < width; i++) {
		for (int j = 0; j < height; j++) {
			gray = qGray(image.pixel(i, j));
			image.setPixel(i, j, qRgb(gray, gray, gray));
		}
	}
    return pixmap.fromImage(image);
}
