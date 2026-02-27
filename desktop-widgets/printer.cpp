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
# include <container_qpainter.h>
# include <litehtml/master_css.h>
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
	QRegularExpression re(QString("([\\d.]+)\\s*%1").arg(unit));
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

	// 3. Insert clearing divs to fix float clearing.
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
		// Insert a clearing div between </table> and any subsequent
		// <table> whose class list does NOT contain a float class.
		QRegularExpression tableTransitionRe(
			"(</table>\\s*)(<table\\s+class=\"([^\"]*)\")");
		int searchPos = 0;
		while (true) {
			QRegularExpressionMatch match = tableTransitionRe.match(html, searchPos);
			if (!match.hasMatch())
				break;
			// Split the class attribute into individual class names
			// and check if any is a float class.
			QStringList classNames = match.captured(3).split(' ', SKIP_EMPTY);
			bool hasFloat = false;
			for (const QString &cls : classNames) {
				if (floatClasses.contains(cls)) {
					hasFloat = true;
					break;
				}
			}
			if (!hasFloat) {
				QString clearDiv = "<div style=\"clear:both;\"></div>";
				int insertPos = match.capturedStart(2);
				html.insert(insertPos, clearDiv);
				searchPos = insertPos + clearDiv.length() + match.captured(2).length();
			} else {
				searchPos = match.capturedEnd();
			}
		}
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

	// For fixed-layout templates (divesPerPage > 0), inject an explicit
	// pixel height on .mainContainer.  The templates use CSS percentage
	// heights (100%, 50%, 33.333%) which require an explicit parent
	// height to resolve — litehtml doesn't assume viewport height as
	// the base like WebKit did.  Without this, dives take their natural
	// content height and page boundaries fall in the middle of dives.
	if (divesPerPage > 0) {
		int containerHeight = pageSize.height() / rows;
		QString heightRule = QString(" .mainContainer { height: %1px; overflow: hidden; }\n")
			.arg(containerHeight);
		html.replace("</style>", heightRule + "</style>");
	}

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

#if defined(USE_QLITEHTML)
// Find all <div> blocks whose class contains "dontbreak".
// Returns a list of (start_position, length) pairs in the HTML string.
static QVector<QPair<int, int>> findDontbreakBlocks(const QString &html)
{
	QVector<QPair<int, int>> blocks;
	QRegularExpression openRe("<div[^>]*class=\"[^\"]*\\bdontbreak\\b[^\"]*\"[^>]*>");
	QRegularExpression tagRe("<(/?)div(?:\\s[^>]*)?>", QRegularExpression::DotMatchesEverythingOption);
	QRegularExpressionMatchIterator openIt = openRe.globalMatch(html);
	while (openIt.hasNext()) {
		QRegularExpressionMatch openMatch = openIt.next();
		int startPos = openMatch.capturedStart();
		int pos = openMatch.capturedEnd();
		int depth = 1;
		QRegularExpressionMatchIterator tagIt = tagRe.globalMatch(html, pos);
		while (tagIt.hasNext() && depth > 0) {
			QRegularExpressionMatch tagMatch = tagIt.next();
			if (tagMatch.captured(1) == "/")
				depth--;
			else
				depth++;
			if (depth == 0)
				blocks.append({startPos, tagMatch.capturedEnd() - startPos});
		}
	}
	return blocks;
}

// For flow layouts (data-numberofdives = 0), measure each dive's
// position in the full rendered document and insert padding before
// dives that would be split across a page boundary.  QLiteHtml's
// print() scrolls through the document in fixed page-sized chunks,
// so the padding pushes dives to the next page cleanly.
//
// Uses DocumentContainer::anchorY() to measure block positions in
// the actual full-document context.
static QString paginateFlowLayout(const QString &html, int pageWidth, int pageHeight,
				  QPaintDevice *paintDevice)
{
	QVector<QPair<int, int>> blocks = findDontbreakBlocks(html);
	if (blocks.isEmpty())
		return html;

	// Add id attributes to each dontbreak div so we can measure their
	// Y positions via anchorY().
	QString markedHtml = html;
	int markOffset = 0;
	QRegularExpression divOpenRe("<div([^>]*)>");
	for (int i = 0; i < blocks.size(); i++) {
		int blockStart = blocks[i].first + markOffset;
		// Find the opening <div...> tag at this block's position
		QRegularExpressionMatch divMatch = divOpenRe.match(markedHtml, blockStart);
		if (divMatch.hasMatch() && divMatch.capturedStart() == blockStart) {
			// Insert id="pbN" into the div's attributes
			QString idAttr = QString(" id=\"pb%1\"").arg(i);
			int insertPos = divMatch.capturedStart(1);
			markedHtml.insert(insertPos, idAttr);
			markOffset += idAttr.length();
		}
	}
	// End marker after the last block
	int endPos = blocks.last().first + blocks.last().second + markOffset;
	markedHtml.insert(endPos, "<div id=\"pb_end\"></div>");

	// Render the full document and read anchor positions.
	// A master stylesheet is required for litehtml to know default
	// display types (div=block, table=table, etc.).  QLiteHtmlWidget
	// sets this internally via its context, but we use a standalone
	// DocumentContainer, so we need to provide it ourselves.
	DocumentContainerContext context;
	context.setMasterStyleSheet(litehtml::master_css);
	DocumentContainer dc;
	dc.setPaintDevice(paintDevice);
	dc.setDataCallback([](const QUrl &url) -> QByteArray {
		if (url.isLocalFile()) {
			QFile file(url.toLocalFile());
			if (file.open(QIODevice::ReadOnly))
				return file.readAll();
		}
		return QByteArray();
	});
	dc.setDocument(markedHtml.toUtf8(), &context);
	dc.render(pageWidth, pageHeight);
	int docHeight = dc.documentHeight();

	QVector<int> positions;
	for (int i = 0; i < blocks.size(); i++)
		positions.append(dc.anchorY(QString("pb%1").arg(i)));
	int endY = dc.anchorY("pb_end");

	// Compute block heights from consecutive anchor positions
	QVector<int> heights;
	for (int i = 0; i < blocks.size(); i++) {
		int nextY = (i + 1 < blocks.size()) ? positions[i + 1] : (endY > 0 ? endY : docHeight);
		heights.append(nextY - positions[i]);
	}

	// Walk forward, simulating the layout with padding insertions.
	// Each padding shifts all subsequent blocks down by exactly the
	// padding amount (the padding div has a fixed pixel height).
	QString result = html;
	int padOffset = 0;    // accumulated string offset from insertions
	int totalPadding = 0; // accumulated pixel shift from padding
	for (int i = 0; i < blocks.size(); i++) {
		int adjustedY = positions[i] + totalPadding;
		int remainingOnPage = pageHeight - (adjustedY % pageHeight);

		// If this block doesn't fit in the remaining page space, pad
		if (adjustedY > 0 && remainingOnPage < pageHeight && heights[i] > remainingOnPage) {
			QString padDiv = QString("<div style=\"height:%1px;\"></div>").arg(remainingOnPage);
			int insertPos = blocks[i].first + padOffset;
			result.insert(insertPos, padDiv);
			padOffset += padDiv.length();
			totalPadding += remainingOnPage;
		}
	}

	return result;
}
#endif // USE_QLITEHTML

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

	// For flow layouts, paginate the preview too so the user can see
	// where page breaks will occur.
	QRegularExpression numDivesRe("data-numberofdives\\s*=\\s*\"?\\s*(\\d+)");
	QRegularExpressionMatch numMatch = numDivesRe.match(content);
	int divesPerPage = numMatch.hasMatch() ? numMatch.captured(1).toInt() : 1;
	if (divesPerPage == 0) {
		QPixmap screenDevice(1, 1);
		content = paginateFlowLayout(content, previewWidth, previewHeight, &screenDevice);
	}

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

	// For flow layouts (data-numberofdives = 0), paginate so that
	// individual dives are not split across page boundaries.
	QRegularExpression numDivesRe("data-numberofdives\\s*=\\s*\"?\\s*(\\d+)");
	QRegularExpressionMatch numMatch = numDivesRe.match(content);
	int divesPerPage = numMatch.hasMatch() ? numMatch.captured(1).toInt() : 1;
	if (divesPerPage == 0)
		content = paginateFlowLayout(content, pageSize.width(), pageSize.height(), printerPtr);

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

