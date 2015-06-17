#include "printer.h"
#include "templatelayout.h"

#include <QtWebKitWidgets>
#include <QPainter>
#include <QWebElementCollection>
#include <QWebElement>

Printer::Printer(QPrinter *printer, print_options *printOptions)
{
	this->printer = printer;
	this->printOptions = printOptions;
	done = 0;
}

void Printer::putProfileImage(QRect profilePlaceholder, QRect viewPort, QPainter *painter, struct dive *dive, QPointer<ProfileWidget2> profile)
{
	int x = profilePlaceholder.x() - viewPort.x();
	int y = profilePlaceholder.y() - viewPort.y();
	// use the placeHolder and the viewPort position to calculate the relative position of the dive profile.
	QRect pos(x, y, profilePlaceholder.width(), profilePlaceholder.height());
	profile->plotDive(dive, true);
	profile->render(painter, pos);
}

void Printer::render()
{
	QPointer<ProfileWidget2> profile = MainWindow::instance()->graphics();

	// keep original preferences
	int profileFrameStyle = profile->frameStyle();
	int animationOriginal = prefs.animation_speed;
	double fontScale = profile->getFontPrintScale();

	// apply printing settings to profile
	profile->setFrameStyle(QFrame::NoFrame);
	profile->setPrintMode(true, !printOptions->color_selected);
	profile->setFontPrintScale(0.6);
	profile->setToolTipVisibile(false);
	prefs.animation_speed = 0;

	// render the Qwebview
	QPainter painter;
	QRect viewPort(0, 0, pageSize.width(), pageSize.height());
	painter.begin(printer);
	painter.setRenderHint(QPainter::Antialiasing);
	painter.setRenderHint(QPainter::SmoothPixmapTransform);

	int divesPerPage;
	switch (printOptions->p_template) {
	case print_options::ONE_DIVE:
		divesPerPage = 1;
		break;
	case print_options::TWO_DIVE:
		divesPerPage = 2;
		break;
	}
	int Pages = ceil(getTotalWork() / (float)divesPerPage);

	// get all refereces to diveprofile class in the Html template
	QWebElementCollection collection = webView->page()->mainFrame()->findAllElements(".diveprofile");

	QSize originalSize = profile->size();
	if (collection.count() > 0) {
		profile->resize(collection.at(0).geometry().size());
	}

	int elemNo = 0;
	for (int i = 0; i < Pages; i++) {
		// render the base Html template
		webView->page()->mainFrame()->render(&painter, QWebFrame::ContentsLayer);

		// render all the dive profiles in the current page
		while (elemNo < collection.count() && collection.at(elemNo).geometry().y() < viewPort.y() + viewPort.height()) {
			// dive id field should be dive_{{dive_no}} se we remove the first 5 characters
			int diveNo = collection.at(elemNo).attribute("id").remove(0, 5).toInt(0, 10);
			putProfileImage(collection.at(elemNo).geometry(), viewPort, &painter, get_dive(diveNo - 1), profile);
			elemNo++;
		}

		// scroll the webview to the next page
		webView->page()->mainFrame()->scroll(0, pageSize.height());
		viewPort.adjust(0, pageSize.height(), 0, pageSize.height());

		// rendering progress is 4/5 of total work
		emit(progessUpdated((i * 80.0 / Pages) + done));
		if (i < Pages - 1)
			printer->newPage();
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
	TemplateLayout t(printOptions);
	webView = new QWebView();
	connect(&t, SIGNAL(progressUpdated(int)), this, SLOT(templateProgessUpdated(int)));

	dpi = printer->resolution();
	//rendering resolution = selected paper size in inchs * printer dpi
	pageSize.setHeight(printer->pageLayout().paintRect(QPageLayout::Inch).height() * dpi);
	pageSize.setWidth(printer->pageLayout().paintRect(QPageLayout::Inch).width() * dpi);
	webView->page()->setViewportSize(pageSize);
	webView->setHtml(t.generate());
	render();
}
