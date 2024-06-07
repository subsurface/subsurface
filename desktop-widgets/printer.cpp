// SPDX-License-Identifier: GPL-2.0
#include "printer.h"
#include "templatelayout.h"
#include "core/divelist.h"
#include "core/divelog.h"
#include "core/selection.h"
#include "core/statistics.h"
#include "core/qthelper.h"
#include "profile-widget/profilescene.h"

#include <algorithm>
#include <memory>
#include <QPainter>
#include <QPrinter>
#include <QtWebKitWidgets>
#include <QWebElementCollection>
#include <QWebElement>

Printer::Printer(QPaintDevice *paintDevice, const print_options &printOptions, const template_options &templateOptions, PrintMode printMode, dive *singleDive) :
	paintDevice(paintDevice),
	webView(new QWebView),
	printOptions(printOptions),
	templateOptions(templateOptions),
	printMode(printMode),
	singleDive(singleDive),
	done(0)
{
}

Printer::~Printer()
{
	delete webView;
}

void Printer::putProfileImage(const QRect &profilePlaceholder, const QRect &viewPort, QPainter *painter,
			      struct dive *dive, ProfileScene *profile)
{
	int x = profilePlaceholder.x() - viewPort.x();
	int y = profilePlaceholder.y() - viewPort.y();
	// use the placeHolder and the viewPort position to calculate the relative position of the dive profile.
	QRect pos(x, y, profilePlaceholder.width(), profilePlaceholder.height());

	profile->draw(painter, pos, dive, 0, nullptr, false);
}

void Printer::flowRender()
{
	// add extra padding at the bottom to pages with height not divisible by view port
	int paddingBottom = pageSize.height() - (webView->page()->mainFrame()->contentsSize().height() % pageSize.height());
	QString styleString = QString::fromUtf8("padding-bottom: ") + QString::number(paddingBottom) + "px;";
	webView->page()->mainFrame()->findFirstElement("body").setAttribute("style", styleString);

	// render the Qwebview
	QPainter painter;
	QRect viewPort(0, 0, 0, 0);
	painter.begin(paintDevice);
	painter.setRenderHint(QPainter::Antialiasing);
	painter.setRenderHint(QPainter::SmoothPixmapTransform);

	// get all references to dontbreak divs
	int start = 0, end = 0;
	int fullPageResolution = webView->page()->mainFrame()->contentsSize().height();
	const QWebElementCollection dontbreak = webView->page()->mainFrame()->findAllElements(".dontbreak");
	for (QWebElement dontbreakElement: dontbreak) {
		if ((dontbreakElement.geometry().y() + dontbreakElement.geometry().height()) - start < pageSize.height()) {
			// One more element can be placed
			end = dontbreakElement.geometry().y() + dontbreakElement.geometry().height();
		} else {
			// fill the page with background color
			QRect fullPage(0, 0, pageSize.width(), pageSize.height());
			QBrush fillBrush(templateOptions.color_palette.color1);
			painter.fillRect(fullPage, fillBrush);
			QRegion reigon(0, 0, pageSize.width(), end - start);
			viewPort.setRect(0, start, pageSize.width(), end - start);

			// render the base Html template
			webView->page()->mainFrame()->render(&painter, QWebFrame::ContentsLayer, reigon);

			// scroll the webview to the next page
			webView->page()->mainFrame()->scroll(0, dontbreakElement.geometry().y() - start);

			// rendering progress is 4/5 of total work
			emit(progessUpdated(lrint((end * 80.0 / fullPageResolution) + done)));

			// add new pages only in print mode, while previewing we don't add new pages
			if (printMode == Printer::PRINT) {
				static_cast<QPrinter*>(paintDevice)->newPage();
			} else {
				painter.end();
				return;
			}
			start = dontbreakElement.geometry().y();
		}
	}
	// render the remianing page
	QRect fullPage(0, 0, pageSize.width(), pageSize.height());
	QBrush fillBrush(templateOptions.color_palette.color1);
	painter.fillRect(fullPage, fillBrush);
	QRegion reigon(0, 0, pageSize.width(), end - start);
	webView->page()->mainFrame()->render(&painter, QWebFrame::ContentsLayer, reigon);

	painter.end();
}

void Printer::render(int pages)
{
	// get all refereces to diveprofile class in the Html template
	QWebElementCollection collection = webView->page()->mainFrame()->findAllElements(".diveprofile");

	// A "standard" profile has about 600 pixels in height.
	// Scale the items in the printed profile accordingly.
	// This is arbitrary, but it seems to work reasonably well.
	double dpr = collection.count() > 0 ? collection[0].geometry().size().height() / 600.0 : 1.0;
	auto profile = std::make_unique<ProfileScene>(dpr, true, !printOptions.color_selected);

	// render the Qwebview
	QPainter painter;
	QRect viewPort(0, 0, pageSize.width(), pageSize.height());
	painter.begin(paintDevice);
	painter.setRenderHint(QPainter::Antialiasing);
	painter.setRenderHint(QPainter::SmoothPixmapTransform);

	int elemNo = 0;
	for (int i = 0; i < pages; i++) {
		// render the base Html template
		webView->page()->mainFrame()->render(&painter, QWebFrame::ContentsLayer);

		// render all the dive profiles in the current page
		while (elemNo < collection.count() && collection.at(elemNo).geometry().y() < viewPort.y() + viewPort.height()) {
			// dive id field should be dive_{{dive_no}} se we remove the first 5 characters
			QString diveIdString = collection.at(elemNo).attribute("id");
			int diveId = diveIdString.remove(0, 5).toInt(0, 10);
			putProfileImage(collection.at(elemNo).geometry(), viewPort, &painter, divelog.dives.get_by_uniq_id(diveId), profile.get());
			elemNo++;
		}

		// scroll the webview to the next page
		webView->page()->mainFrame()->scroll(0, pageSize.height());
		viewPort.adjust(0, pageSize.height(), 0, pageSize.height());

		// rendering progress is 4/5 of total work
		emit(progessUpdated(lrint((i * 80.0 / pages) + done)));
		if (i < pages - 1 && printMode == Printer::PRINT)
			static_cast<QPrinter*>(paintDevice)->newPage();
	}
	painter.end();
}

//value: ranges from 0 : 100 and shows the progress of the templating engine
void Printer::templateProgessUpdated(int value)
{
	done = value / 5; //template progess if 1/5 of total work
	emit progessUpdated(done);
}

std::vector<dive *> Printer::getDives() const
{
	if (singleDive) {
		return { singleDive };
	} else if (printOptions.print_selected) {
		return getDiveSelection();
	} else {
		std::vector<dive *> res;
		for (auto &d: divelog.dives)
			res.push_back(d.get());
		return res;
	}
}

QString Printer::exportHtml()
{
	TemplateLayout t(printOptions, templateOptions);
	connect(&t, SIGNAL(progressUpdated(int)), this, SLOT(templateProgessUpdated(int)));
	QString html;

	if (printOptions.type == print_options::DIVELIST)
		html = t.generate(getDives());
	else if (printOptions.type == print_options::STATISTICS )
		html = t.generateStatistics();

	// TODO: write html to file
	return html;
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
	int dpi = printerPtr->resolution();
	//rendering resolution = selected paper size in inchs * printer dpi
	pageSize.setHeight(qCeil(printerPtr->pageRect(QPrinter::Inch).height() * dpi));
	pageSize.setWidth(qCeil(printerPtr->pageRect(QPrinter::Inch).width() * dpi));
	webView->page()->setViewportSize(pageSize);
	webView->page()->mainFrame()->setScrollBarPolicy(Qt::Vertical, Qt::ScrollBarAlwaysOff);

	// export border width with at least 1 pixel
	// templateOptions.borderwidth = std::max(1, pageSize.width() / 1000);
	if (printOptions.type == print_options::DIVELIST)
		webView->setHtml(t.generate(getDives()));
	else if (printOptions.type == print_options::STATISTICS )
		webView->setHtml(t.generateStatistics());
	if (printOptions.color_selected && printerPtr->colorMode())
		printerPtr->setColorMode(QPrinter::Color);
	else
		printerPtr->setColorMode(QPrinter::GrayScale);
	// apply user settings
	int divesPerPage;

	// get number of dives per page from data-numberofdives attribute in the body of the selected template
	bool ok;
	divesPerPage = webView->page()->mainFrame()->findFirstElement("body").attribute("data-numberofdives").toInt(&ok);
	if (!ok) {
		divesPerPage = 1; // print each dive in a single page if the attribute is missing or malformed
		//TODO: show warning
	}
	if (divesPerPage == 0)
		flowRender();
	else
		render((t.numDives - 1) / divesPerPage + 1);
}

void Printer::previewOnePage()
{
	if (printMode == PREVIEW) {
		TemplateLayout t(printOptions, templateOptions);

		pageSize.setHeight(paintDevice->height());
		pageSize.setWidth(paintDevice->width());
		webView->page()->setViewportSize(pageSize);
		// initialize the border settings
		// templateOptions.border_width = std::max(1, pageSize.width() / 1000);
		if (printOptions.type == print_options::DIVELIST)
			webView->setHtml(t.generate(getDives()));
		else if (printOptions.type == print_options::STATISTICS )
			webView->setHtml(t.generateStatistics());
		bool ok;
		int divesPerPage = webView->page()->mainFrame()->findFirstElement("body").attribute("data-numberofdives").toInt(&ok);
		if (!ok) {
			divesPerPage = 1; // print each dive in a single page if the attribute is missing or malformed
			//TODO: show warning
		}
		if (divesPerPage == 0) {
			flowRender();
		} else {
			render(1);
		}
	}
}
