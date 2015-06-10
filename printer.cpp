#include "printer.h"
#include "templatelayout.h"

#include <QtWebKitWidgets>
#include <QPainter>
#include <QWebElementCollection>
#include <QWebElement>

#define A4_300DPI_WIDTH 2480
#define A4_300DPI_HIGHT 3508

Printer::Printer(QPrinter *printer)
{
	this->printer = printer;

	//override these settings for now.
	printer->setFullPage(true);
	printer->setOrientation(QPrinter::Portrait);
	printer->setPaperSize(QPrinter::A4);
	printer->setPrintRange(QPrinter::AllPages);
	printer->setResolution(300);
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
	profile->setPrintMode(true);
	profile->setFontPrintScale(0.6);
	profile->setToolTipVisibile(false);
	prefs.animation_speed = 0;

	// render the Qwebview
	QPainter painter;
	QSize size(A4_300DPI_WIDTH, A4_300DPI_HIGHT);
	QRect viewPort(0, 0, size.width(), size.height());
	painter.begin(printer);
	painter.setRenderHint(QPainter::Antialiasing);
	painter.setRenderHint(QPainter::SmoothPixmapTransform);

	webView->page()->setViewportSize(size);
	int Pages = ceil(getTotalWork() / 2.0);

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
		webView->page()->mainFrame()->scroll(0, size.height());
		viewPort.adjust(0, size.height(), 0, size.height());

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
	TemplateLayout t;
	connect(&t, SIGNAL(progressUpdated(int)), this, SLOT(templateProgessUpdated(int)));
	webView = new QWebView();
	webView->setHtml(t.generate());
	render();
}
