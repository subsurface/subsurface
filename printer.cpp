#include "printer.h"
#include "templatelayout.h"
#include "statistics.h"
#include "helpers.h"

#include <algorithm>
#include <QtWebKitWidgets>
#include <QPainter>
#include <QWebElementCollection>
#include <QWebElement>

Printer::Printer(QPaintDevice *paintDevice, print_options *printOptions, template_options *templateOptions,  PrintMode printMode)
{
	this->paintDevice = paintDevice;
	this->printOptions = printOptions;
	this->templateOptions = templateOptions;
	this->printMode = printMode;
	dpi = 0;
	done = 0;
	webView = new QWebView();
}

Printer::~Printer()
{
	delete webView;
}

void Printer::putProfileImage(QRect profilePlaceholder, QRect viewPort, QPainter *painter, struct dive *dive, QPointer<ProfileWidget2> profile)
{
	int x = profilePlaceholder.x() - viewPort.x();
	int y = profilePlaceholder.y() - viewPort.y();
	// use the placeHolder and the viewPort position to calculate the relative position of the dive profile.
	QRect pos(x, y, profilePlaceholder.width(), profilePlaceholder.height());
	profile->plotDive(dive, true);

	if (!printOptions->color_selected) {
		QImage image(pos.width(), pos.height(), QImage::Format_ARGB32);
		QPainter imgPainter(&image);
		imgPainter.setRenderHint(QPainter::Antialiasing);
		imgPainter.setRenderHint(QPainter::SmoothPixmapTransform);
		profile->render(&imgPainter, QRect(0, 0, pos.width(), pos.height()));
		imgPainter.end();

		// convert QImage to grayscale before rendering
		for (int i = 0; i < image.height(); i++) {
			QRgb *pixel = reinterpret_cast<QRgb *>(image.scanLine(i));
			QRgb *end = pixel + image.width();
			for (; pixel != end; pixel++) {
				int gray_val = qGray(*pixel);
				*pixel = QColor(gray_val, gray_val, gray_val).rgb();
			}
		}

		painter->drawImage(pos, image);
	} else {
		profile->render(painter, pos);
	}
}

void Printer::flowRender()
{
	// render the Qwebview
	QPainter painter;
	QRect viewPort(0, 0, 0, 0);
	painter.begin(paintDevice);
	painter.setRenderHint(QPainter::Antialiasing);
	painter.setRenderHint(QPainter::SmoothPixmapTransform);

	// get all references to dontbreak divs
	int start = 0, end = 0;
	int fullPageResolution = webView->page()->mainFrame()->contentsSize().height();
	QWebElementCollection dontbreak = webView->page()->mainFrame()->findAllElements(".dontbreak");
	foreach (QWebElement dontbreakElement, dontbreak) {
		if ((dontbreakElement.geometry().y() + dontbreakElement.geometry().height()) - start < pageSize.height()) {
			// One more element can be placed
			end = dontbreakElement.geometry().y() + dontbreakElement.geometry().height();
		} else {
			// fill the page with background color
			QRect fullPage(0, 0, pageSize.width(), pageSize.height());
			QBrush fillBrush(templateOptions->color_palette.color1);
			painter.fillRect(fullPage, fillBrush);
			QRegion reigon(0, 0, pageSize.width(), end - start);
			viewPort.setRect(0, start, pageSize.width(), end - start);

			// render the base Html template
			webView->page()->mainFrame()->render(&painter, QWebFrame::ContentsLayer, reigon);

			// scroll the webview to the next page
			webView->page()->mainFrame()->scroll(0, dontbreakElement.geometry().y() - start);

			// rendering progress is 4/5 of total work
			emit(progessUpdated((end * 80.0 / fullPageResolution) + done));
			static_cast<QPrinter*>(paintDevice)->newPage();
			start = dontbreakElement.geometry().y();
		}
	}
	// render the remianing page
	QRect fullPage(0, 0, pageSize.width(), pageSize.height());
	QBrush fillBrush(templateOptions->color_palette.color1);
	painter.fillRect(fullPage, fillBrush);
	QRegion reigon(0, 0, pageSize.width(), end - start);
	webView->page()->mainFrame()->render(&painter, QWebFrame::ContentsLayer, reigon);

	painter.end();
}

void Printer::render(int Pages = 0)
{
	// keep original preferences
	QPointer<ProfileWidget2> profile = MainWindow::instance()->graphics();
	int profileFrameStyle = profile->frameStyle();
	int animationOriginal = prefs.animation_speed;
	double fontScale = profile->getFontPrintScale();
	double printFontScale = 1.0;

	// apply printing settings to profile
	profile->setFrameStyle(QFrame::NoFrame);
	profile->setPrintMode(true, !printOptions->color_selected);
	profile->setToolTipVisibile(false);
	prefs.animation_speed = 0;

	// render the Qwebview
	QPainter painter;
	QRect viewPort(0, 0, pageSize.width(), pageSize.height());
	painter.begin(paintDevice);
	painter.setRenderHint(QPainter::Antialiasing);
	painter.setRenderHint(QPainter::SmoothPixmapTransform);

	// get all refereces to diveprofile class in the Html template
	QWebElementCollection collection = webView->page()->mainFrame()->findAllElements(".diveprofile");

	QSize originalSize = profile->size();
	if (collection.count() > 0) {
		printFontScale = (double)collection.at(0).geometry().size().height() / (double)profile->size().height();
		profile->resize(collection.at(0).geometry().size());
	}
	profile->setFontPrintScale(printFontScale);

	int elemNo = 0;
	for (int i = 0; i < Pages; i++) {
		// render the base Html template
		webView->page()->mainFrame()->render(&painter, QWebFrame::ContentsLayer);

		// render all the dive profiles in the current page
		while (elemNo < collection.count() && collection.at(elemNo).geometry().y() < viewPort.y() + viewPort.height()) {
			// dive id field should be dive_{{dive_no}} se we remove the first 5 characters
			QString diveIdString = collection.at(elemNo).attribute("id");
			int diveId = diveIdString.remove(0, 5).toInt(0, 10);
			putProfileImage(collection.at(elemNo).geometry(), viewPort, &painter, get_dive_by_uniq_id(diveId), profile);
			elemNo++;
		}

		// scroll the webview to the next page
		webView->page()->mainFrame()->scroll(0, pageSize.height());
		viewPort.adjust(0, pageSize.height(), 0, pageSize.height());

		// rendering progress is 4/5 of total work
		emit(progessUpdated((i * 80.0 / Pages) + done));
		if (i < Pages - 1 && printMode == Printer::PRINT)
			static_cast<QPrinter*>(paintDevice)->newPage();
	}
	painter.end();

	// return profle settings
	profile->setFrameStyle(profileFrameStyle);
	profile->setPrintMode(false);
	profile->setFontPrintScale(fontScale);
	profile->setToolTipVisibile(true);
	profile->resize(originalSize);
	prefs.animation_speed = animationOriginal;

	//replot the dive after returning the settings
	profile->plotDive(0, true);
}

//value: ranges from 0 : 100 and shows the progress of the templating engine
void Printer::templateProgessUpdated(int value)
{
	done = value / 5; //template progess if 1/5 of total work
	emit progessUpdated(done);
}

void Printer::print()
{
	// we can only print if "PRINT" mode is selected
	if (printMode != Printer::PRINT) {
		return;
	}

	QPrinter *printerPtr;
	printerPtr = static_cast<QPrinter*>(paintDevice);

	TemplateLayout t(printOptions, templateOptions);
	connect(&t, SIGNAL(progressUpdated(int)), this, SLOT(templateProgessUpdated(int)));
	dpi = printerPtr->resolution();
	//rendering resolution = selected paper size in inchs * printer dpi
	pageSize.setHeight(std::ceil(printerPtr->pageRect(QPrinter::Inch).height() * dpi));
	pageSize.setWidth(std::ceil(printerPtr->pageRect(QPrinter::Inch).width() * dpi));
	webView->page()->setViewportSize(pageSize);
	webView->page()->mainFrame()->setScrollBarPolicy(Qt::Vertical, Qt::ScrollBarAlwaysOff);
	// export border width with at least 1 pixel
	templateOptions->border_width = std::max(1, pageSize.width() / 1000);
	webView->setHtml(t.generate());
	if (printOptions->color_selected && printerPtr->colorMode()) {
		printerPtr->setColorMode(QPrinter::Color);
	} else {
		printerPtr->setColorMode(QPrinter::GrayScale);
	}
	// apply user settings
	int divesPerPage;

	// get number of dives per page from data-numberofdives attribute in the body of the selected template
	bool ok;
	divesPerPage = webView->page()->mainFrame()->findFirstElement("body").attribute("data-numberofdives").toInt(&ok);
	if (!ok) {
		divesPerPage = 1; // print each dive in a single page if the attribute is missing or malformed
		//TODO: show warning
	}
	int Pages;
	if (divesPerPage == 0) {
		// add extra padding at the bottom to pages with height not divisible by view port
		int paddingBottom = pageSize.height() - (webView->page()->mainFrame()->contentsSize().height() % pageSize.height());
		QString styleString = QString::fromUtf8("padding-bottom: ") + QString::number(paddingBottom) + "px;";
		webView->page()->mainFrame()->findFirstElement("body").setAttribute("style", styleString);
		flowRender();
	} else {
		Pages = ceil(getTotalWork(printOptions) / (float)divesPerPage);
		render(Pages);
	}
}

void Printer::print_statistics()
{
	QPrinter *printerPtr;
	printerPtr = static_cast<QPrinter*>(paintDevice);
	stats_t total_stats;

	total_stats.selection_size = 0;
	total_stats.total_time.seconds = 0;

	QString html;
	html += "<table border=1>";
	html += "<tr>";
	html += "<td>Year</td>";
	html += "<td>Dives</td>";
	html += "<td>Total Time</td>";
	html += "<td>Avg Time</td>";
	html += "<td>Shortest Time</td>";
	html += "<td>Longest Time</td>";
	html += "<td>Avg Depth</td>";
	html += "<td>Min Depth</td>";
	html += "<td>Max Depth</td>";
	html += "<td>Avg SAC</td>";
	html += "<td>Min SAC</td>";
	html += "<td>Max SAC</td>";
	html += "<td>Min Temp</td>";
	html += "<td>Max Temp</td>";
	html += "</tr>";
	int i = 0;
	while (stats_yearly != NULL && stats_yearly[i].period) {
		html += "<tr>";
		html += "<td>" + QString::number(stats_yearly[i].period) + "</td>";
		html += "<td>" + QString::number(stats_yearly[i].selection_size) + "</td>";
		html += "<td>" + QString::fromUtf8(get_time_string(stats_yearly[i].total_time.seconds, 0)) + "</td>";
		html += "<td>" + QString::fromUtf8(get_minutes(stats_yearly[i].total_time.seconds / stats_yearly[i].selection_size)) + "</td>";
		html += "<td>" + QString::fromUtf8(get_minutes(stats_yearly[i].shortest_time.seconds)) + "</td>";
		html += "<td>" + QString::fromUtf8(get_minutes(stats_yearly[i].longest_time.seconds)) + "</td>";
		html += "<td>" + get_depth_string(stats_yearly[i].avg_depth) + "</td>";
		html += "<td>" + get_depth_string(stats_yearly[i].min_depth) + "</td>";
		html += "<td>" + get_depth_string(stats_yearly[i].max_depth) + "</td>";
		html += "<td>" + get_volume_string(stats_yearly[i].avg_sac) + "</td>";
		html += "<td>" + get_volume_string(stats_yearly[i].min_sac) + "</td>";
		html += "<td>" + get_volume_string(stats_yearly[i].max_sac) + "</td>";
		html += "<td>" + QString::number(stats_yearly[i].min_temp == 0 ? 0 : get_temp_units(stats_yearly[i].min_temp, NULL)) + "</td>";
		html += "<td>" + QString::number(stats_yearly[i].max_temp == 0 ? 0 : get_temp_units(stats_yearly[i].max_temp, NULL)) + "</td>";
		html += "</tr>";
		total_stats.selection_size += stats_yearly[i].selection_size;
		total_stats.total_time.seconds += stats_yearly[i].total_time.seconds;
		i++;
	}
	html += "</table>";
	webView->setHtml(html);
	webView->print(printerPtr);
}

void Printer::previewOnePage()
{
	if (printMode == PREVIEW) {
		TemplateLayout t(printOptions, templateOptions);

		pageSize.setHeight(paintDevice->height());
		pageSize.setWidth(paintDevice->width());
		webView->page()->setViewportSize(pageSize);
		webView->setHtml(t.generate());

		// render only one page
		render(1);
	}
}
