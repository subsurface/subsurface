#include "printer.h"
#include "templatelayout.h"

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
	pageSize.setHeight(printerPtr->pageRect(QPrinter::Inch).height() * dpi);
	pageSize.setWidth(printerPtr->pageRect(QPrinter::Inch).width() * dpi);
	webView->page()->setViewportSize(pageSize);
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
		Pages = ceil(webView->page()->mainFrame()->contentsSize().height() / (float)pageSize.height());
	} else {
		Pages = ceil(getTotalWork(printOptions) / (float)divesPerPage);
	}
	render(Pages);
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
