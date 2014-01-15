#include <QtCore/qmath.h>
#include <QDebug>
#include <QPainter>
#include <QDesktopWidget>
#include <QApplication>
#include <QTableView>
#include <QHeaderView>
#include "mainwindow.h"
#include "profilegraphics.h"
#include "../dive.h"
#include "../display.h"
#include "printdialog.h"
#include "printlayout.h"
#include "models.h"
#include "modeldelegates.h"

PrintLayout::PrintLayout(PrintDialog *dialogPtr, QPrinter *printerPtr, struct options *optionsPtr)
{
	dialog = dialogPtr;
	printer = printerPtr;
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
	// profile print settings
	const int dw = 20; // base percentage
	profilePrintColumnWidths.append(dw);
	profilePrintColumnWidths.append(dw);
	profilePrintColumnWidths.append(dw - 3);
	profilePrintColumnWidths.append(dw - 3);
	profilePrintColumnWidths.append(dw + 6); // fit to 100%
	const int sr = 12; // smallest row height in pixels
	profilePrintRowHeights.append(sr);
	profilePrintRowHeights.append(sr + 4);
	profilePrintRowHeights.append(sr);
	profilePrintRowHeights.append(sr);
	profilePrintRowHeights.append(sr);
	profilePrintRowHeights.append(sr);
	profilePrintRowHeights.append(sr);
	profilePrintRowHeights.append(sr);
	profilePrintRowHeights.append(sr);
	profilePrintRowHeights.append(sr);
	profilePrintRowHeights.append(sr);
	profilePrintRowHeights.append(sr);
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

// go trought the dive table and find how many dives we are a going to print
int PrintLayout::estimateTotalDives() const
{
	int total = 0, i = 0;
	struct dive *dive;
	for_each_dive(i, dive) {
		if (!dive->selected && printOptions->print_selected)
			continue;
		total++;
	}
	return total;
}

/* the used formula here is:
 * s = (S - (n - 1) * p) / n
 * where:
 * s is the length of a single element (unknown)
 * S is the total available length
 * n is the number of elements to fit
 * p is the padding between elements
 */
#define ESTIMATE_DIVE_DIM(S, n, p) \
	 ((S) - ((n) - 1) * (p)) / (n);

void PrintLayout::printProfileDives(int divesPerRow, int divesPerColumn)
{
	int i, row = 0, col = 0, printed = 0, total = estimateTotalDives();
	struct dive *dive;
	if (!total)
		return;

	// setup a painter
	QPainter painter;
	painter.begin(printer);
	painter.setRenderHint(QPainter::Antialiasing);
	painter.setRenderHint(QPainter::SmoothPixmapTransform);
	painter.scale(scaleX, scaleY);

	// setup the profile widget
	ProfileGraphicsView *profile = mainWindow()->graphics();
	const int profileFrameStyle = profile->frameStyle();
	profile->setFrameStyle(QFrame::NoFrame);
	profile->clear();
	profile->setPrintMode(true, !printOptions->color_selected);
	QSize originalSize = profile->size();
	// swap rows/col for landscape
	if (printer->orientation() == QPrinter::Landscape) {
		int swap = divesPerColumn;
		divesPerColumn = divesPerRow;
		divesPerRow = swap;
	}
	// padding in pixels between two dives. no padding if only one dive per page.
	const int padDef = 20;
	const int padW = (divesPerColumn < 2) ? 0 : padDef;
	const int padH = (divesPerRow < 2) ? 0 : padDef;
	// estimate dimensions for a single dive
	const int scaledW = ESTIMATE_DIVE_DIM(scaledPageW, divesPerColumn, padW);
	const int scaledH = ESTIMATE_DIVE_DIM(scaledPageH, divesPerRow, padH);
	// padding in pixels between profile and table
	const int padPT = 5;
	// create a model and table
	ProfilePrintModel model;
	QTableView *table = createProfileTable(&model, scaledW);
	// profilePrintTableMaxH updates after the table is created
	const int tableH = profilePrintTableMaxH;
	// resize the profile widget
	profile->resize(scaledW, scaledH - tableH - padPT);
	// offset table or profile on top
	int yOffsetProfile = 0, yOffsetTable = 0;
	if (printOptions->notes_up)
		yOffsetProfile = tableH + padPT;
	else
		yOffsetTable = scaledH - tableH;

	// plot the dives at specific rows and columns on the page
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
		QTransform origTransform = painter.transform();

		// draw a profile
		painter.translate((scaledW + padW) * col, (scaledH + padH) * row + yOffsetProfile);
		profile->plot(dive, true);
		profile->render(&painter, QRect(0, 0, scaledW, scaledH - tableH - padPT));
		painter.setTransform(origTransform);

		// draw a table
		painter.translate((scaledW + padW) * col, (scaledH + padH) * row + yOffsetTable);
		model.setDive(dive);
		table->render(&painter);
		painter.setTransform(origTransform);
		col++;
		printed++;
		emit signalProgress((printed * 100) / total);
	}

	// cleanup
	painter.end();
	delete table;
	profile->setFrameStyle(profileFrameStyle);
	profile->setPrintMode(false);
	profile->resize(originalSize);
	profile->clear();
	profile->plot(current_dive, true);
}

/* we create a table that has a fixed height, but can stretch to fit certain width */
QTableView *PrintLayout::createProfileTable(ProfilePrintModel *model, const int tableW)
{
	// setup a new table
	QTableView *table = new QTableView();
	QHeaderView *vHeader = table->verticalHeader();
	QHeaderView *hHeader = table->horizontalHeader();
	table->setAttribute(Qt::WA_DontShowOnScreen);
	table->setSelectionMode(QAbstractItemView::NoSelection);
	table->setFocusPolicy(Qt::NoFocus);
	table->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	table->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	hHeader->setVisible(false);
	vHeader->setVisible(false);
#if QT_VERSION < QT_VERSION_CHECK(5,0,0)
	hHeader->setResizeMode(QHeaderView::Fixed);
	vHeader->setResizeMode(QHeaderView::Fixed);
#else
	hHeader->setSectionResizeMode(QHeaderView::Fixed);
	vHeader->setSectionResizeMode(QHeaderView::Fixed);
#endif
	// set the model
	table->setModel(model);

	/* setup cell span for the table using QTableView::setSpan().
	 * changes made here reflect on ProfilePrintModel::data(). */
	const int cols = model->columnCount();
	const int rows = model->rowCount();
	// info on top
	table->setSpan(0, 0, 1, 4);
	table->setSpan(1, 0, 1, 4);
	// gas used
	table->setSpan(2, 0, 1, 2);
	table->setSpan(3, 0, 1, 2);
	// notes
	table->setSpan(6, 0, 1, 5);
	table->setSpan(7, 0, 5, 5);

	/* resize row heights to the 'profilePrintRowHeights' indexes.
	 * profilePrintTableMaxH will then hold the table height. */
	int i;
	profilePrintTableMaxH = 0;
	for (i = 0; i < rows; i++) {
		int h = profilePrintRowHeights.at(i);
		profilePrintTableMaxH += h;
		vHeader->resizeSection(i, h);
	}
	// resize columns. columns widths are percentages from the table width.
	int accW = 0;
	for (i = 0; i < cols; i++) {
		int pw = qCeil((qreal)(profilePrintColumnWidths.at(i) * tableW) / 100.0);
		accW += pw;
		if (i == cols - 1 && accW > tableW) /* adjust last column */
			pw -= accW - tableW;
		hHeader->resizeSection(i, pw);
	}
	// resize
	table->resize(tableW, profilePrintTableMaxH);
	// hide the grid and set a stylesheet
	table->setItemDelegate(new ProfilePrintDelegate());
	table->setShowGrid(false);
	table->setStyleSheet(
		"QTableView { border: none }"
		"QTableView::item { border: 0px; padding-left: 2px; padding-right: 2px; }"
	);
	// return
	return table;
}

void PrintLayout::printTable()
{
	struct dive *dive;
	int done = 0; // percents done
	int i, row = 0, progress, total = estimateTotalDives();
	if (!total)
		return;

	// create and setup a table
	QTableView table;
	table.setAttribute(Qt::WA_DontShowOnScreen);
	table.setSelectionMode(QAbstractItemView::NoSelection);
	table.setFocusPolicy(Qt::NoFocus);
	table.horizontalHeader()->setVisible(false);
	table.verticalHeader()->setVisible(false);
#if QT_VERSION < QT_VERSION_CHECK(5,0,0)
	table.horizontalHeader()->setResizeMode(QHeaderView::Fixed);
	table.verticalHeader()->setResizeMode(QHeaderView::ResizeToContents);
#else
	table.horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);
	table.verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
#endif
	table.setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	table.setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	// fit table to one page initially
	table.resize(scaledPageW,  scaledPageH);

	// don't show border
	table.setStyleSheet(
		"QTableView { border: none }"
	);

	// create and fill a table model
	TablePrintModel model;
	addTablePrintHeadingRow(&model, row); // add one heading row
	row++;
	progress = 0;
	for_each_dive(i, dive) {
		if (!dive->selected && printOptions->print_selected)
			continue;
		addTablePrintDataRow(&model, row, dive);
		row++;
		progress++;
		emit signalProgress((progress * 10) / total);
	}
	done = 10;
	table.setModel(&model); // set model to table
	// resize columns to percentages from page width
	int accW = 0;
	int cols = model.columns;
	int tableW = table.width();
	for (int i = 0; i < model.columns; i++) {
		int pw = qCeil((qreal)(tablePrintColumnWidths.at(i) * table.width()) / 100.0);
		accW += pw;
		if (i == cols - 1 && accW > tableW) /* adjust last column */
			pw -= accW - tableW;
		table.horizontalHeader()->resizeSection(i, pw);
	}
	// reset the model at this point
	model.callReset();

	// a list of vertical offsets where pages begin and some helpers
	QList<unsigned int> pageIndexes;
	pageIndexes.append(0);

	/* the algorithm bellow processes the table rows in multiple passes,
	 * compensating for loss of space due to moving rows on a new page instead
	 * of truncating them.
	 * there is a 'passes' array defining how much percents of the total
	 * progress each will take. given, the first and last stage of this function
	 * use 10% each, then the sum of passes[] here should be 80%.
	 * two should be enough! */
	const int passes[] = { 70, 10 };
	int tableHeight = 0, lastAccIndex = 0, rowH, accH, headings;
	bool isHeading = false;

	for (unsigned int pass = 0; pass < sizeof(passes) / sizeof(passes[0]); pass++) {
		progress = headings = accH = 0;
		total = model.rows - lastAccIndex;
		for (int i = lastAccIndex; i < model.rows; i++) {
			rowH = table.rowHeight(i);
			accH += rowH;
			if (isHeading) {
				headings += rowH;
				isHeading = false;
			}
			if (accH > scaledPageH) {
				lastAccIndex = i;
				pageIndexes.append(pageIndexes.last() + (accH - rowH));
				addTablePrintHeadingRow(&model, i);
				isHeading = true;
				accH = 0;
				i--;
			}
			tableHeight += table.rowHeight(i);
			progress++;
			emit signalProgress(done + (progress * passes[pass]) / total);
		}
		done += passes[pass];
	}
	done = 90;
	pageIndexes.append(pageIndexes.last() + accH + headings);
	table.resize(scaledPageW, tableHeight);

	// attach a painter and render pages by using pageIndexes
	QPainter painter(printer);
	painter.setRenderHint(QPainter::Antialiasing);
	painter.setRenderHint(QPainter::SmoothPixmapTransform);
	painter.scale(scaleX, scaleY);
	total = pageIndexes.size() - 1;
	progress = 0;
	for (int i = 0; i < total; i++) {
		if (i > 0)
			printer->newPage();
		QRegion region(0, pageIndexes.at(i) - 1,
			       table.width(),
			       pageIndexes.at(i + 1) - pageIndexes.at(i) + 1);
		table.render(&painter, QPoint(0, 0), region);
		progress++;
		emit signalProgress(done + (progress * 10) / total);
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
