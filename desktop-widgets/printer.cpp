// SPDX-License-Identifier: GPL-2.0
#include "printer.h"
#include "core/dive.h"
#include "backend-shared/exportfuncs.h"
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
#include "qmath.h"
#if defined(USE_QLITEHTML)
# include <QUrl>
# include <QFile>
# include <QVBoxLayout>
# include <qlitehtmlwidget.h>
#else
#include <QtWebKitWidgets>
#include <QWebElementCollection>
#include <QWebElement>
#endif

Printer::Printer(QPaintDevice *paintDevice, const print_options &printOptions, const template_options &templateOptions, PrintMode printMode, dive *singleDive) :
	paintDevice(paintDevice),
#ifndef USE_QLITEHTML
	webView(new QWebView),
#endif
	printOptions(printOptions),
	templateOptions(templateOptions),
	printMode(printMode),
	singleDive(singleDive),
	done(0)
{
}

Printer::~Printer()
{
#ifndef USE_QLITEHTML
	delete webView;
#endif
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
#ifndef USE_QLITEHTML
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
#endif
}

void Printer::render(int pages)
{
#ifndef USE_QLITEHTML
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
#endif
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

// Helper: replace CSS viewport units (vw or vh) with px values
// computed from the actual page/viewport dimension.
// 1vw = pageWidth/100, 1vh = pageHeight/100.
static QString replaceViewportUnit(const QString &html, const QString &unit, int dimension)
{
	QRegularExpression re(QString("([\\d.]+)%1").arg(unit));
	QString result;
	int lastEnd = 0;
	QRegularExpressionMatchIterator it = re.globalMatch(html);
	while (it.hasNext()) {
		QRegularExpressionMatch match = it.next();
		result += html.mid(lastEnd, match.capturedStart() - lastEnd);
		double value = match.captured(1).toDouble();
		int px = qRound(value * dimension / 100.0);
		result += QString::number(px) + "px";
		lastEnd = match.capturedEnd();
	}
	result += html.mid(lastEnd);
	return result;
}

// Adapt HTML/CSS from printing templates for litehtml compatibility.
// This allows existing user-created templates (designed for WebKit) to
// work without modification under QLiteHtml/litehtml.
// pageWidth/pageHeight are the rendering dimensions in pixels so that
// viewport units (vw/vh) can be converted to concrete px values.
static QString adaptForLiteHtml(QString html, int pageWidth, int pageHeight)
{
	// 1. Replace viewport units (not supported by litehtml) with px
	//    computed from the actual rendering dimensions.  This makes
	//    the output correct for both the on-screen preview (≈1000px)
	//    and the printer (e.g. 4500px at 600 dpi on letter paper).
	html = replaceViewportUnit(html, "vw", pageWidth);
	html = replaceViewportUnit(html, "vh", pageHeight);

	// 2. Strip -webkit- vendor prefixes
	html.replace("-webkit-box-sizing", "box-sizing");
	html.replace(QRegularExpression("-webkit-filter\\s*:"), "filter:");
	html.replace(QRegularExpression("-webkit-column-break-inside\\s*:[^;]*;"), "");

	// 3. Inject CSS overrides before </style>:
	//    - Remove float from h1/p (commonly floated in templates for
	//      WebKit table cell layout, but causes overlap in litehtml).
	//      Don't use !important so inline styles (e.g. float:right)
	//      still take precedence.
	//    - Reset diveProfile height (templates set percentage heights
	//      for empty placeholder divs; with actual images, auto is correct).
	html.replace("</style>",
		"\n\t\th1 { float: none; }\n"
		"\t\tp { float: none; }\n"
		"\t\t.diveProfile { height: auto; }\n"
		"\t</style>");

	// 4. Insert clearing divs to fix float clearing.
	//    litehtml doesn't support the overflow:hidden BFC trick that
	//    WebKit templates use for float clearing. Instead, find CSS
	//    classes that use float: and insert an explicit clearing div
	//    in the HTML before any sibling table/div that does NOT use
	//    one of those float classes.
	QSet<QString> floatClasses;
	QRegularExpression floatClassRe("\\.([\\w-]+)\\s*\\{[^}]*float\\s*:");
	QRegularExpressionMatchIterator it = floatClassRe.globalMatch(html);
	while (it.hasNext())
		floatClasses.insert(it.next().captured(1));

	if (!floatClasses.isEmpty()) {
		// Build a pattern that matches the closing tag of a floated element
		// followed by a table/div that does NOT have one of the float classes.
		// We look for </table> followed by <table class="non-float-class">.
		QString floatClassAlternation;
		for (const QString &cls : floatClasses) {
			if (!floatClassAlternation.isEmpty())
				floatClassAlternation += "|";
			floatClassAlternation += QRegularExpression::escape(cls);
		}
		// Match </table> (with whitespace) followed by <table class="...">
		// where the class does NOT contain any float class name.
		// We use a negative lookahead to exclude float classes.
		QRegularExpression afterFloatRe(
			QString("(</table>\\s*)"
				"(<table\\s+class=\"(?:(?!%1)[^\"]*)\")").arg(floatClassAlternation));
		html.replace(afterFloatRe, "\\1<div style=\"clear:both;\"></div>\\2");
	}

	return html;
}

QString Printer::generateContent()
{
	std::unique_ptr<ProfileScene> profile = getPrintProfile();
	for (struct dive *dive : getDives())
		exportProfile(*profile, *dive, printDir.filePath(QString("dive_%1.png").arg(dive->id)), false);

	TemplateLayout t(printOptions, templateOptions);
	connect(&t, SIGNAL(progressUpdated(int)), this, SLOT(templateProgessUpdated(int)));

	QString html;
	if (printOptions.type == print_options::DIVELIST)
		html = t.generate(getDives());
	else if (printOptions.type == print_options::STATISTICS)
		html = t.generateStatistics();

	// Compute a max-height for profile images based on page dimensions
	// and the number of dives per page (from data-numberofdives).
	// The profile should take about 40% of each cell's height.
	QRegularExpression numDivesRe("data-numberofdives\\s*=\\s*\"?\\s*(\\d+)");
	QRegularExpressionMatch numMatch = numDivesRe.match(html);
	int divesPerPage = numMatch.hasMatch() ? numMatch.captured(1).toInt() : 1;
	int rows;
	if (divesPerPage <= 0)
		rows = 3; // flow layout: assume ~3 dives visible
	else if (divesPerPage <= 2)
		rows = divesPerPage;
	else
		rows = (divesPerPage + 1) / 2; // grid: 2 columns
	int profileMaxHeight = pageSize.height() * 40 / (100 * rows);

	// Replace dive profile divs with profile images, keeping the div
	// wrapper so that the template's .diveProfile CSS (float, margins)
	// still applies.  Use max-width/max-height to constrain the image
	// proportionally — the image picks whichever is more restrictive
	// while maintaining its aspect ratio.
	QString repl = QString("<div class=\"diveProfile\" id=\"dive_\\1\">"
			       "<img style=\"max-width:100%%; max-height:%1px;\" src=\"file:///%2/dive_\\1.png\">"
			       "</div>").arg(profileMaxHeight).arg(printDir.path());
	html.replace(QRegularExpression("<div\\s+class=\"diveProfile\"\\s+id=\"dive_([^\"]*)\"\\s*>\\s*</div>"), repl);

	// Adapt the HTML for litehtml compatibility.
	// pageSize must be set before calling generateContent() —
	// preview() sets it to the widget dimensions, print() sets it
	// from the printer resolution.
	html = adaptForLiteHtml(html, pageSize.width(), pageSize.height());

	return html;
}

void Printer::preview()
{
#ifdef USE_QLITEHTML
	// Use the printer's actual page dimensions to compute the preview
	// size, so the preview matches the printed page's aspect ratio.
	QPrinter *printerPtr = static_cast<QPrinter*>(paintDevice);
	QRectF pageRect = printerPtr->pageRect(QPrinter::Inch);
	double aspectRatio = pageRect.height() / pageRect.width();
	int previewWidth = 800;
	int previewHeight = qRound(previewWidth * aspectRatio);

	pageSize = QSize(previewWidth, previewHeight);
	QString content = generateContent();

	QDialog previewer;
	previewer.setWindowTitle(tr("Print Preview"));
	previewer.setWindowIcon(QIcon(":subsurface-icon"));

	QVBoxLayout *layout = new QVBoxLayout(&previewer);
	QLiteHtmlWidget *previewWidget = new QLiteHtmlWidget(&previewer);
	previewWidget->setResourceHandler([](const QUrl &url) -> QByteArray {
		if (url.isLocalFile()) {
			QFile file(url.toLocalFile());
			if (file.open(QIODevice::ReadOnly)) {
				QByteArray data = file.readAll();
				file.close();
				return data;
			}
		}
		return QByteArray();
	});
	previewWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	layout->addWidget(previewWidget);

	previewWidget->setUrl(QUrl("file:///", QUrl::TolerantMode));
	previewWidget->setHtml(content);
	previewer.resize(previewWidth, previewHeight);
	previewer.exec();
#endif
}

void Printer::print()
{
	// we can only print if "PRINT" mode is selected
	if (printMode != Printer::PRINT) {
		return;
	}

	QPrinter *printerPtr = static_cast<QPrinter*>(paintDevice);

	int dpi = printerPtr->resolution();
	//rendering resolution = selected paper size in inchs * printer dpi
	pageSize.setHeight(qCeil(printerPtr->pageRect(QPrinter::Inch).height() * dpi));
	pageSize.setWidth(qCeil(printerPtr->pageRect(QPrinter::Inch).width() * dpi));
#ifdef USE_QLITEHTML
	QString content = generateContent();

	QLiteHtmlWidget printWidget;
	printWidget.setResourceHandler([](const QUrl &url) -> QByteArray {
		if (url.isLocalFile()) {
			QFile file(url.toLocalFile());
			if (file.open(QIODevice::ReadOnly)) {
				QByteArray data = file.readAll();
				file.close();
				return data;
			}
		}
		return QByteArray();
	});
	printWidget.setUrl(QUrl("file:///", QUrl::TolerantMode));
	printWidget.setHtml(content);
	printWidget.print(printerPtr);
#else
	std::unique_ptr<ProfileScene> profile = getPrintProfile();
	for (struct dive *dive : getDives())
		exportProfile(*profile, *dive, printDir.filePath(QString("dive_%1.png").arg(dive->id)), false);

	TemplateLayout t(printOptions, templateOptions);
	connect(&t, SIGNAL(progressUpdated(int)), this, SLOT(templateProgessUpdated(int)));

	webView->page()->setViewportSize(pageSize);
	webView->page()->mainFrame()->setScrollBarPolicy(Qt::Vertical, Qt::ScrollBarAlwaysOff);

	if (printOptions.type == print_options::DIVELIST)
		webView->setHtml(t.generate(getDives()));
	else if (printOptions.type == print_options::STATISTICS )
		webView->setHtml(t.generateStatistics());
	if (printOptions.color_selected && printerPtr->colorMode())
		printerPtr->setColorMode(QPrinter::Color);
	else
		printerPtr->setColorMode(QPrinter::GrayScale);
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
#endif
}

void Printer::previewOnePage()
{
	if (printMode == PREVIEW) {
		TemplateLayout t(printOptions, templateOptions);

		pageSize.setHeight(paintDevice->height());
		pageSize.setWidth(paintDevice->width());
#ifndef USE_QLITEHTML
		webView->page()->setViewportSize(pageSize);
#endif
		// initialize the border settings
		// templateOptions.border_width = std::max(1, pageSize.width() / 1000);
#ifndef USE_QLITEHTML
		if (printOptions.type == print_options::DIVELIST)
			webView->setHtml(t.generate(getDives()));
		else if (printOptions.type == print_options::STATISTICS )
			webView->setHtml(t.generateStatistics());
#endif
		bool ok;
		int divesPerPage;
#ifndef USE_QLITEHTML
		divesPerPage = webView->page()->mainFrame()->findFirstElement("body").attribute("data-numberofdives").toInt(&ok);
#else
		divesPerPage = 1;   // FIXME
		ok = true;
#endif
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

