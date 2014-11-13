#include <QtCore/qmath.h>
#include <QDebug>
#include <QPainter>
#include <QDesktopWidget>
#include <QApplication>
#include <QTableView>
#include <QHeaderView>
#include <QPointer>
#include <QPicture>
#include <QMessageBox>

#include "mainwindow.h"
#include "../dive.h"
#include "../display.h"
#include "printdialog.h"
#include "printlayout.h"
#include "models.h"
#include "modeldelegates.h"

PrintLayout::PrintLayout(PrintDialog *dialogPtr, QPrinter *printerPtr, struct print_options *optionsPtr)
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
	tablePrintColumnWidths.append(14);
	tablePrintColumnWidths.append(8);
	tablePrintColumnWidths.append(8);
	tablePrintColumnWidths.append(15);
	tablePrintColumnWidths.append(15);
	tablePrintColumnWidths.append(33);
	// profile print settings
	const int dw = 20; // base percentage
	profilePrintColumnWidths.append(dw);
	profilePrintColumnWidths.append(dw);
	profilePrintColumnWidths.append(dw + 8);
	profilePrintColumnWidths.append(dw - 4);
	profilePrintColumnWidths.append(dw - 4); // fit to 100%
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
	if (pageW == 0 || pageH == 0) {
		QMessageBox msgBox;
		msgBox.setIcon(QMessageBox::Critical);
		msgBox.setText(tr("Subsurface cannot find a usable printer on this system!"));
		msgBox.setWindowIcon(QIcon(":subsurface-icon"));
		msgBox.exec();
		return;
	}
	switch (printOptions->type) {
	case print_options::PRETTY:
		printProfileDives(3, 2);
		break;
	case print_options::ONEPERPAGE:
		printProfileDives(1, 1);
		break;
	case print_options::TWOPERPAGE:
		printProfileDives(2, 1);
		break;
	case print_options::TABLE:
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

	// a printer page in pixels
	pageW = pageRect.width();
	pageH = pageRect.height();
}

// go trought the dive table and find how many dives we are a going to print
// TODO: C function: 'count_selected_dives' or something
int PrintLayout::estimateTotalDives() const
{
	int total = 0, i = 0;
	struct dive *dive;
	for_each_dive (i, dive) {
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
	int animationOriginal = prefs.animation_speed;

	struct dive *dive;
	if (!total)
		return;

	// disable animations on the profile:
	prefs.animation_speed = 0;

	// setup a painter
	QPainter painter;
	painter.begin(printer);
	painter.setRenderHint(QPainter::Antialiasing);
	painter.setRenderHint(QPainter::SmoothPixmapTransform);

	// setup the profile widget
	QPointer<ProfileWidget2> profile = MainWindow::instance()->graphics();
	const int profileFrameStyle = profile->frameStyle();
	profile->setFrameStyle(QFrame::NoFrame);
	profile->setPrintMode(true, !printOptions->color_selected);
	profile->setFontPrintScale(divesPerRow * divesPerColumn > 3 ? 0.6 : 1.0);
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
	const int scaledW = ESTIMATE_DIVE_DIM(pageW, divesPerColumn, padW);
	const int scaledH = ESTIMATE_DIVE_DIM(pageH, divesPerRow, padH);
	// padding in pixels between profile and table
	const int padPT = 5;
	// create a model and table
	ProfilePrintModel model;
	model.setFontsize(7); // if this is changed we also need to change 'const int sr' in the constructor
	// if there is only one dive per page row we pass fitNotesToHeight to be almost half the page height
	QPointer<QTableView> table(createProfileTable(&model, scaledW, (divesPerRow == 1) ? scaledH * 0.45 : 0.0));
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
	for_each_dive (i, dive) {
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
		// draw a profile
		QTransform origTransform = painter.transform();
		painter.translate((scaledW + padW) * col, (scaledH + padH) * row + yOffsetProfile);
		profile->plotDive(dive, true); // make sure the profile is actually redrawn
#ifdef Q_OS_LINUX // on Linux there is a vector line bug (big lines in PDF), which forces us to render to QImage
		QImage image(scaledW, scaledH - tableH - padPT, QImage::Format_ARGB32);
		QPainter imgPainter(&image);
		imgPainter.setRenderHint(QPainter::Antialiasing);
		imgPainter.setRenderHint(QPainter::SmoothPixmapTransform);
		profile->render(&imgPainter, QRect(0, 0, scaledW, scaledH - tableH - padPT));
		imgPainter.end();
		painter.drawImage(image.rect(),image);
#else // for other OS we can try rendering the profile as vector
		profile->render(&painter, QRect(0, 0, scaledW, scaledH - tableH - padPT));
#endif
		painter.setTransform(origTransform);

		// draw a table
		QPicture pic;
		QPainter picPainter;
		painter.translate((scaledW + padW) * col, (scaledH + padH) * row + yOffsetTable);
		model.setDive(dive);
		picPainter.begin(&pic);
		table->render(&picPainter);
		picPainter.end();
		painter.drawPicture(QPoint(0,0), pic);
		painter.setTransform(origTransform);
		col++;
		printed++;
		emit signalProgress((printed * 100) / total);
	}
	// cleanup
	painter.end();
	profile->setFrameStyle(profileFrameStyle);
	profile->setPrintMode(false);
	profile->resize(originalSize);
	// we need to force a redraw of the profile so it switches back from print mode
	profile->plotDive(0, true);
	// re-enable animations
	prefs.animation_speed = animationOriginal;
}

/* we create a table that has a fixed height, but can stretch to fit certain width */
QTableView *PrintLayout::createProfileTable(ProfilePrintModel *model, const int tableW, const qreal fitNotesToHeight)
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
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
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
	table->setSpan(0, 0, 1, 3);
	table->setSpan(1, 0, 1, 3);
	table->setSpan(0, 3, 1, 2);
	table->setSpan(1, 3, 1, 2);
	// gas used
	table->setSpan(2, 0, 1, 2);
	table->setSpan(3, 0, 1, 2);
	// notes
	table->setSpan(6, 0, 1, 5);
	table->setSpan(7, 0, 5, 5);
	/* resize row heights to the 'profilePrintRowHeights' indexes.
	 * profilePrintTableMaxH will then hold the table height.
	 * what fitNotesToHeight does it to expand the notes section to fit a special height */
	int i;
	profilePrintTableMaxH = 0;
	for (i = 0; i < rows; i++) {
		int h = (i == rows - 1 && fitNotesToHeight != 0.0) ? fitNotesToHeight : profilePrintRowHeights.at(i);
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
	table->setItemDelegate(new ProfilePrintDelegate(table));
	table->setItemDelegateForRow(7, new HTMLDelegate(table));

	table->setShowGrid(false);
	table->setStyleSheet(
		"QTableView { border: none }"
		"QTableView::item { border: 0px; padding-left: 2px; padding-right: 2px; }");
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
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
	table.horizontalHeader()->setResizeMode(QHeaderView::Fixed);
	table.verticalHeader()->setResizeMode(QHeaderView::ResizeToContents);
#else
	table.horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);
	table.verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
#endif
	table.setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	table.setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	// fit table to one page initially
	table.resize(pageW, pageH);

	// don't show border
	table.setStyleSheet(
		"QTableView { border: none }");

	// create and fill a table model
	TablePrintModel model;
	addTablePrintHeadingRow(&model, row); // add one heading row
	row++;
	progress = 0;
	for_each_dive (i, dive) {
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
	for (i = 0; i < model.columns; i++) {
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
	int tableHeight = 0, lastAccIndex = 0, rowH, accH, headings, headingRowHeightD2, headingRowHeight;
	bool newHeading = false;

	for (unsigned int pass = 0; pass < sizeof(passes) / sizeof(passes[0]); pass++) {
		progress = headings = accH = 0;
		total = model.rows - lastAccIndex;
		for (i = lastAccIndex; i < model.rows; i++) {
			rowH = table.rowHeight(i);
			if (i == 0) { // first row is always a heading. it's height is constant.
				headingRowHeight = rowH;
				headingRowHeightD2 = rowH / 2;
			}
			if (rowH > pageH - headingRowHeight) // skip huge rows. we don't support row spanning on multiple pages.
				continue;
			accH += rowH;
			if (newHeading) {
				headings += rowH;
				newHeading = false;
			}
			if (accH > pageH) {
				lastAccIndex = i;
				pageIndexes.append(pageIndexes.last() + (accH - rowH));
				addTablePrintHeadingRow(&model, i);
				newHeading = true;
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
	table.resize(pageW, tableHeight);

	/* attach a painter and render pages by using pageIndexes
	 * there is a weird QPicture dependency here; we need to offset a page
	 * by headingRowHeightD2, which is half the heading height. the same doesn't
	 * make sense if we are rendering the table widget directly to the printer-painter. */
	QPainter painter(printer);
	painter.setRenderHint(QPainter::Antialiasing);
	painter.setRenderHint(QPainter::SmoothPixmapTransform);
	total = pageIndexes.size() - 1;
	progress = 0;
	for (i = 0; i < total; i++) {
		if (i > 0)
			printer->newPage();
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
		(void)headingRowHeightD2;
		QRegion region(0, pageIndexes.at(i) - 1,
			       table.width(),
			       pageIndexes.at(i + 1) - pageIndexes.at(i) + 1);
		table.render(&painter, QPoint(0, 0), region);
#else
		QRegion region(0, pageIndexes.at(i) + headingRowHeightD2 - 1,
			       table.width(),
			       pageIndexes.at(i + 1) - (pageIndexes.at(i) + headingRowHeightD2) + 1);
		// vectorize the table first by using QPicture
		QPicture pic;
		QPainter picPainter;
		picPainter.begin(&pic);
		table.render(&picPainter, QPoint(0, 0), region);
		picPainter.end();
		painter.drawPicture(QPoint(0, headingRowHeightD2), pic);
#endif
		progress++;
		emit signalProgress(done + (progress * 10) / total);
	}
}

void PrintLayout::addTablePrintDataRow(TablePrintModel *model, int row, struct dive *dive) const
{
	struct DiveItem di;
	di.diveId = dive->id;
	model->insertRow();
	model->setData(model->index(row, 0), QString::number(dive->number), Qt::DisplayRole);
	model->setData(model->index(row, 1), di.displayDate(), Qt::DisplayRole);
	model->setData(model->index(row, 2), di.displayDepthWithUnit(), Qt::DisplayRole);
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
