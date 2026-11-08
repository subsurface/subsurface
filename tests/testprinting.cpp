// SPDX-License-Identifier: GPL-2.0
#include "testprinting.h"

#include "core/dive.h"
#include "core/divelog.h"
#include "core/file.h"
#include "core/pref.h"
#include "core/qthelper.h"
#include "desktop-widgets/printer.h"
#include "desktop-widgets/printoptions.h"
#include "profile-widget/profilescene.h"

#include <QApplication>
#include <QDir>
#include <QFile>
#include <QImage>
#include <QPageSize>
#include <QPrinter>
#include <QRegularExpression>
#include <QTemporaryDir>
#include <QTextStream>
#include <QTimer>
#include <QUuid>
#if defined(USE_QLITEHTML)
#include <qlitehtmlwidget.h>
#endif

class ProfileAnimation {
};

// Lightweight ProfileScene implementation for this test target.
// It keeps TestPrinting self-contained while still exercising printer.cpp.
ProfileScene::ProfileScene(double dpr, bool printMode, bool isGrayscale) :
	d(nullptr),
	dc(0),
	dpr(dpr),
	printMode(printMode),
	isGrayscale(isGrayscale),
	empty(true),
	maxtime(0),
	maxdepth(0),
	profileYAxis(nullptr),
	gasYAxis(nullptr),
	temperatureAxis(nullptr),
	timeAxis(nullptr),
	cylinderPressureAxis(nullptr),
	heartBeatAxis(nullptr),
	percentageAxis(nullptr),
	diveProfileItem(nullptr),
	temperatureItem(nullptr),
	meanDepthItem(nullptr),
	gasPressureItem(nullptr),
	diveComputerText(nullptr),
	reportedCeiling(nullptr),
	pn2GasItem(nullptr),
	pheGasItem(nullptr),
	po2GasItem(nullptr),
	o2SetpointGasItem(nullptr),
	ccrsensor1GasItem(nullptr),
	ccrsensor2GasItem(nullptr),
	ccrsensor3GasItem(nullptr),
	ocpo2GasItem(nullptr),
	diveCeiling(nullptr),
	decoModelParameters(nullptr),
	heartBeatItem(nullptr),
	percentageItem(nullptr),
	tankItem(nullptr)
{
}

ProfileScene::~ProfileScene() = default;

void ProfileScene::resize(QSizeF)
{
}

void ProfileScene::clear()
{
}

bool ProfileScene::pointOnProfile(const QPointF &) const
{
	return false;
}

void ProfileScene::anim(double)
{
}

void ProfileScene::plotDive(const struct dive *, int, DivePlannerPointsModel *, bool, bool, bool, bool, double, double)
{
}

void ProfileScene::draw(QPainter *painter, const QRect &pos, const struct dive *, int, DivePlannerPointsModel *, bool)
{
	painter->save();
	painter->setPen(Qt::black);
	painter->drawLine(pos.topLeft(), pos.bottomRight());
	painter->drawLine(pos.bottomLeft(), pos.topRight());
	painter->restore();
}

double ProfileScene::calcZoomPosition(double, double originalPos, double)
{
	return originalPos;
}

void TestPrinting::initTestCase()
{
	TestBase::initTestCase();
	prefs = default_prefs;
}

void TestPrinting::cleanup()
{
	for (const QString &name: temporaryTemplates)
		removeTemplate(name);
	temporaryTemplates.clear();
	clear_dive_file_data();
}

QString TestPrinting::loadFileContents(const QString &path) const
{
	QFile file(path);
	if (!file.open(QFile::ReadOnly | QFile::Text))
		return QString();
	QTextStream stream(&file);
	return stream.readAll();
}

QString TestPrinting::createTemplateFromString(const QString &prefix, const QString &contents) const
{
	const QString templateName = QString("%1_%2.html").arg(prefix).arg(QUuid::createUuid().toString(QUuid::Id128));
	QDir().mkpath(getPrintingTemplatePathUser());
	QFile templateFile(getPrintingTemplatePathUser() + QDir::separator() + templateName);
	if (!templateFile.open(QFile::WriteOnly | QFile::Text))
		return QString();
	QTextStream stream(&templateFile);
	stream << contents;
	templateFile.close();
	return templateName;
}

QString TestPrinting::createTemplateFromFile(const QString &prefix, const QString &sourcePath) const
{
	const QString contents = loadFileContents(sourcePath);
	if (contents.isEmpty())
		return QString();
	return createTemplateFromString(prefix, contents);
}

static std::pair<print_options, template_options> defaultTestOptions(const QString &templateName)
{
	print_options printOptions{};
	printOptions.type = print_options::DIVELIST;
	printOptions.p_template = templateName;
	printOptions.print_selected = true;
	printOptions.color_selected = true;
	printOptions.landscape = false;
	printOptions.resolution = 300;

	template_options templateOptions{};
	templateOptions.font_index = 0;
	templateOptions.color_palette_index = SSRF_COLORS;
	templateOptions.border_width = 1;
	templateOptions.font_size = 9;
	templateOptions.line_spacing = 1;
	templateOptions.color_palette.color1 = QColor::fromRgb(0xff, 0xff, 0xff);
	templateOptions.color_palette.color2 = QColor::fromRgb(0xa6, 0xbc, 0xd7);
	templateOptions.color_palette.color3 = QColor::fromRgb(0xef, 0xf7, 0xff);
	templateOptions.color_palette.color4 = QColor::fromRgb(0x34, 0x65, 0xa4);
	templateOptions.color_palette.color5 = QColor::fromRgb(0x20, 0x4a, 0x87);
	templateOptions.color_palette.color6 = QColor::fromRgb(0x17, 0x37, 0x64);

	return {printOptions, templateOptions};
}

QString TestPrinting::exportHtmlWithTemplate(const QString &templateName, dive *singleDive, bool allDives) const
{
	auto [printOptions, templateOptions] = defaultTestOptions(templateName);
	if (allDives)
		printOptions.print_selected = false; // print the whole dive log

	QImage image(1200, 800, QImage::Format_ARGB32);
	image.fill(Qt::white);

	if (divelog.dives.empty())
		return QString();
	if (!allDives && !singleDive)
		singleDive = divelog.dives[0].get();
	Printer printer(&image, printOptions, templateOptions, Printer::PREVIEW, singleDive);
	return printer.exportHtml();
}

// The text of every "dontbreak" container of the flow layout, from its opening tag to the next
// container or the end of the document, without markup. Each container holds one dive.
static QStringList flowContainerTexts(const QString &html)
{
	const QRegularExpression container("<div[^>]*class=\"(?:[^\"]*\\s)?dontbreak(?:\\s[^\"]*)?\"[^>]*>");
	const QRegularExpression tags("<[^>]*>");
	const QRegularExpression whitespace("\\s+");
	QList<qsizetype> starts;
	auto matches = container.globalMatch(html);
	while (matches.hasNext())
		starts << matches.next().capturedEnd();
	QStringList texts;
	for (qsizetype i = 0; i < starts.size(); ++i) {
		const qsizetype end = i + 1 < starts.size() ? starts[i + 1] : html.size();
		texts << html.mid(starts[i], end - starts[i]).remove(tags).replace(whitespace, " ").trimmed();
	}
	return texts;
}

// What every dive looks like in the flow layout, from exporting the dives one by one.
QStringList TestPrinting::flowTextsPerDive(const QString &templateName) const
{
	QStringList texts;
	for (const auto &dive : divelog.dives)
		texts << flowContainerTexts(exportHtmlWithTemplate(templateName, dive.get())).value(0);
	return texts;
}

QImage TestPrinting::renderWithTemplate(const QString &templateName) const
{
	auto [printOptions, templateOptions] = defaultTestOptions(templateName);

	QImage image(1200, 800, QImage::Format_ARGB32);
	image.fill(Qt::white);

	if (divelog.dives.empty())
		return QImage();
	Printer printer(&image, printOptions, templateOptions, Printer::PREVIEW, divelog.dives[0].get());
	printer.previewOnePage();
	return image;
}

void TestPrinting::removeTemplate(const QString &templateName) const
{
	QFile::remove(getPrintingTemplatePathUser() + QDir::separator() + templateName);
}

int TestPrinting::imageDifferencePixels(const QImage &a, const QImage &b) const
{
	if (a.size() != b.size())
		return -1;
	int differences = 0;
	const int width = a.width();
	const int height = a.height();
	for (int y = 0; y < height; ++y) {
		const QRgb *lineA = reinterpret_cast<const QRgb *>(a.constScanLine(y));
		const QRgb *lineB = reinterpret_cast<const QRgb *>(b.constScanLine(y));
		for (int x = 0; x < width; ++x) {
			if (lineA[x] != lineB[x])
				++differences;
		}
	}
	return differences;
}

void TestPrinting::testPrinterExportHtmlDiveId_data()
{
	QTest::addColumn<QString>("templatePath");

	QTest::newRow("bundled-one-dive") << QString(SUBSURFACE_SOURCE "/printing_templates/One Dive.html");
	QTest::newRow("bundled-two-dives") << QString(SUBSURFACE_SOURCE "/printing_templates/Two Dives.html");
	QTest::newRow("bundled-six-dives") << QString(SUBSURFACE_SOURCE "/printing_templates/Six Dives.html");

	QTest::newRow("custom-self-closing") << QString(SUBSURFACE_SOURCE "/tests/printing_templates/custom_variant_self_closing.html");
	QTest::newRow("custom-extra-classes") << QString(SUBSURFACE_SOURCE "/tests/printing_templates/custom_variant_extra_classes.html");
	QTest::newRow("custom-style-attributes") << QString(SUBSURFACE_SOURCE "/tests/printing_templates/custom_variant_style_attributes.html");
}

void TestPrinting::testPrinterExportHtmlDiveId()
{
	QFETCH(QString, templatePath);

	QCOMPARE(parse_file(SUBSURFACE_TEST_DATA "/dives/abitofeverything.ssrf", &divelog), 0);
	QVERIFY(!divelog.dives.empty());
	const int diveId = divelog.dives[0]->id;

	const QString prefix = templatePath.startsWith("/printing_templates/")
		? QString("test_print_bundled")
		: QString("test_print_custom");
	const QString templateName = createTemplateFromFile(prefix, templatePath);
	QVERIFY2(!templateName.isEmpty(), qPrintable(templatePath));
	temporaryTemplates << templateName;

	const QString html = exportHtmlWithTemplate(templateName);
	QVERIFY2(!html.isEmpty(), qPrintable(templatePath));
	QVERIFY2(html.contains(QString("dive_%1").arg(diveId)), qPrintable(templatePath));
	QVERIFY2(!html.contains("{{ dive.id }}"), qPrintable(templatePath));
}

void TestPrinting::testPrinterRenderRecognizesDiveProfile_data()
{
	QTest::addColumn<QString>("templatePath");

	QTest::newRow("custom-self-closing") << QString(SUBSURFACE_SOURCE "/tests/printing_templates/custom_variant_self_closing.html");
	QTest::newRow("custom-extra-classes") << QString(SUBSURFACE_SOURCE "/tests/printing_templates/custom_variant_extra_classes.html");
	QTest::newRow("custom-style-attributes") << QString(SUBSURFACE_SOURCE "/tests/printing_templates/custom_variant_style_attributes.html");
}

void TestPrinting::testPrinterRenderRecognizesDiveProfile()
{
	QFETCH(QString, templatePath);

	QCOMPARE(parse_file(SUBSURFACE_TEST_DATA "/dives/abitofeverything.ssrf", &divelog), 0);
	QVERIFY(!divelog.dives.empty());

	const QString recognizedContents = loadFileContents(templatePath);
	QVERIFY2(!recognizedContents.isEmpty(), qPrintable(templatePath));
	const QString controlContents = QString(recognizedContents).replace("diveProfile", "notDiveProfile");

	const QString recognizedTemplate = createTemplateFromString("test_print_render_yes", recognizedContents);
	QVERIFY2(!recognizedTemplate.isEmpty(), qPrintable(templatePath));
	temporaryTemplates << recognizedTemplate;
	const QString controlTemplate = createTemplateFromString("test_print_render_no", controlContents);
	QVERIFY2(!controlTemplate.isEmpty(), qPrintable(templatePath));
	temporaryTemplates << controlTemplate;

	const QImage recognized = renderWithTemplate(recognizedTemplate);
	const QImage control = renderWithTemplate(controlTemplate);
	QVERIFY2(!recognized.isNull(), qPrintable(templatePath));
	QVERIFY2(!control.isNull(), qPrintable(templatePath));

	const int changedPixels = imageDifferencePixels(recognized, control);
	QVERIFY2(changedPixels >= 0, qPrintable(QString("%1: image size mismatch").arg(templatePath)));
	QVERIFY2(changedPixels > 200, qPrintable(QString("%1 changedPixels=%2").arg(templatePath).arg(changedPixels)));
}

void TestPrinting::testBundledLayout_data()
{
	QTest::addColumn<QString>("fileName");
	QTest::addColumn<QString>("divesPerPage");
	QTest::newRow("one") << QString("One Dive.html") << QString("1");
	QTest::newRow("two") << QString("Two Dives.html") << QString("2");
	QTest::newRow("six") << QString("Six Dives.html") << QString("6");
	QTest::newRow("flow") << QString("Flowlayout.html") << QString("0");
}

void TestPrinting::testBundledLayout()
{
	QFETCH(QString, fileName);
	QFETCH(QString, divesPerPage);
	QCOMPARE(parse_file(SUBSURFACE_TEST_DATA "/dives/abitofeverything.ssrf", &divelog), 0);
	QVERIFY(!divelog.dives.empty());
	const QString name = createTemplateFromFile("test_print_layout", QString(SUBSURFACE_SOURCE "/printing_templates/") + fileName);
	QVERIFY2(!name.isEmpty(), qPrintable(fileName));
	temporaryTemplates << name;

	// Export the whole log so that the multi-dive and flow layouts have dives to lay out.
	const QString html = exportHtmlWithTemplate(name, nullptr, true);
	QVERIFY2(QRegularExpression(QString("data-numberofdives\\s*=\\s*\\\"?%1\\\"?").arg(divesPerPage)).match(html).hasMatch(),
		 qPrintable(fileName));
	if (fileName == "Flowlayout.html") {
		// Every dive has its own "dontbreak" container, so the containers must show the dives
		// one by one. Counting them would not notice a repeated and an omitted dive.
		const QStringList expectedTexts = flowTextsPerDive(name);
		// A repeated dive can only be told from an omitted one if the dives look different.
		QVERIFY(!expectedTexts.contains(QString()));
		QStringList distinctTexts = expectedTexts;
		distinctTexts.removeDuplicates();
		QCOMPARE(distinctTexts.size(), int(divelog.dives.size()));
		QVERIFY2(flowContainerTexts(html) == expectedTexts, qPrintable(fileName));
	} else {
		for (const auto &dive : divelog.dives)
			QVERIFY2(html.contains(QString("id=\"dive_%1\"").arg(dive->id)),
				 qPrintable(QString("%1: dive %2 is missing").arg(fileName).arg(dive->id)));
	}
	// The render check below only draws the first dive.
	QImage rendered = renderWithTemplate(name);
	QVERIFY2(!rendered.isNull(), qPrintable(fileName));
	QImage blank(rendered.size(), rendered.format());
	blank.fill(Qt::white);
	QVERIFY2(imageDifferencePixels(rendered, blank) > 200, qPrintable(fileName));
}

void TestPrinting::testPdfPages_data()
{
	QTest::addColumn<QString>("fileName");
	QTest::addColumn<int>("paper");
	QTest::addColumn<int>("divesPerPage");
	QTest::newRow("one-A4") << QString("One Dive.html") << int(QPageSize::A4) << 1;
	QTest::newRow("two-Letter") << QString("Two Dives.html") << int(QPageSize::Letter) << 2;
	QTest::newRow("six-A4") << QString("Six Dives.html") << int(QPageSize::A4) << 6;
	QTest::newRow("flow-Letter") << QString("Flowlayout.html") << int(QPageSize::Letter) << 0;
}

void TestPrinting::testPdfPages()
{
	QFETCH(QString, fileName);
	QFETCH(int, paper);
	QFETCH(int, divesPerPage);
	QCOMPARE(parse_file(SUBSURFACE_TEST_DATA "/dives/abitofeverything.ssrf", &divelog), 0);
	QVERIFY(!divelog.dives.empty());
	const QString name = createTemplateFromFile("test_print_pdf", QString(SUBSURFACE_SOURCE "/printing_templates/") + fileName);
	QVERIFY(!name.isEmpty());
	temporaryTemplates << name;
	QTemporaryDir output;
	QVERIFY(output.isValid());
	const QString pdf = output.filePath("layout.pdf");
	QPrinter device(QPrinter::HighResolution);
	device.setOutputFormat(QPrinter::PdfFormat);
	device.setOutputFileName(pdf);
	device.setPageSize(QPageSize(static_cast<QPageSize::PageSizeId>(paper)));
	device.setResolution(96);
	auto [printOptions, templateOptions] = defaultTestOptions(name);
	printOptions.print_selected = false;
	Printer printer(&device, printOptions, templateOptions, Printer::PRINT, nullptr);
	printer.print();
	QFile result(pdf);
	QVERIFY2(result.exists(), qPrintable(pdf));
	QVERIFY2(result.size() > 1000, qPrintable(pdf));
	QVERIFY(result.open(QIODevice::ReadOnly));
	const QByteArray contents = result.readAll();
	QVERIFY(contents.startsWith("%PDF-"));
	const QRegularExpression pageMarker("/Type\\s*/Page\\b");
	int pages = 0;
	// Qt 5's fromLatin1(QByteArray) stops at the first NUL byte, which the PDF's
	// binary streams contain, so convert the whole buffer using its explicit size.
	const QString text = QString::fromLatin1(contents.constData(), contents.size());
	auto matches = pageMarker.globalMatch(text);
	while (matches.hasNext()) {
		matches.next();
		++pages;
	}
	// A fixed layout starts a new page after every divesPerPage dives, so a PDF with
	// fewer pages is missing dives. The renderer may use more pages if a layout
	// overflows, so this is a lower bound. The flow layout paginates by content height
	// and is only expected to span more than one page.
	const int dives = int(divelog.dives.size());
	const int minimumPages = divesPerPage > 0 ? (dives + divesPerPage - 1) / divesPerPage : 2;
	QVERIFY2(pages >= minimumPages,
		 qPrintable(QString("%1: %2 pages, expected at least %3").arg(fileName).arg(pages).arg(minimumPages)));
#if defined(USE_QLITEHTML)
	// The page text is stored as glyph streams that cannot be searched, but litehtml embeds
	// the profile of every dive in a fixed layout as an image, so a dive missing from the PDF
	// means too few images. The flow layout has no profile images and keeps the page check.
	if (divesPerPage > 0) {
		int images = 0;
		for (qsizetype pos = 0; (pos = contents.indexOf("/Subtype /Image", pos)) >= 0; ++pos)
			++images;
		QVERIFY2(images >= dives,
			 qPrintable(QString("%1: %2 images, expected at least %3").arg(fileName).arg(images).arg(dives)));
	}
#endif
}

#if defined(USE_QLITEHTML)
void TestPrinting::testPreviewDialog_data()
{
	QTest::addColumn<QString>("fileName");
	QTest::addColumn<int>("paper");
	QTest::newRow("one-A4") << QString("One Dive.html") << int(QPageSize::A4);
	QTest::newRow("two-Letter") << QString("Two Dives.html") << int(QPageSize::Letter);
	QTest::newRow("six-A4") << QString("Six Dives.html") << int(QPageSize::A4);
	QTest::newRow("flow-Letter") << QString("Flowlayout.html") << int(QPageSize::Letter);
}

void TestPrinting::testPreviewDialog()
{
	QFETCH(QString, fileName);
	QFETCH(int, paper);
	QCOMPARE(parse_file(SUBSURFACE_TEST_DATA "/dives/abitofeverything.ssrf", &divelog), 0);
	QVERIFY(!divelog.dives.empty());
	const QString name = createTemplateFromFile("test_print_preview", QString(SUBSURFACE_SOURCE "/printing_templates/") + fileName);
	QVERIFY(!name.isEmpty());
	temporaryTemplates << name;
	QPrinter device(QPrinter::HighResolution);
	device.setPageSize(QPageSize(static_cast<QPageSize::PageSizeId>(paper)));
	auto [printOptions, templateOptions] = defaultTestOptions(name);
	printOptions.print_selected = false;
	Printer printer(&device, printOptions, templateOptions, Printer::PREVIEW, nullptr);
	QStringList expectedTexts;
	if (fileName == "Flowlayout.html") {
		expectedTexts = flowTextsPerDive(name);
		// A repeated dive can only be told from an omitted one if the dives look different.
		QVERIFY(!expectedTexts.contains(QString()));
		QStringList distinctTexts = expectedTexts;
		distinctTexts.removeDuplicates();
		QCOMPARE(distinctTexts.size(), int(divelog.dives.size()));
	}
	bool opened = false;
	bool hasContent = false;
	QTimer::singleShot(0, [&] {
		QWidget *dialog = QApplication::activeModalWidget();
		if (!dialog)
			return;
		opened = true;
		auto *view = dialog->findChild<QLiteHtmlWidget *>();
		hasContent = view && view->isVisible();
		if (hasContent) {
			const QString html = view->html();
			if (fileName == "Flowlayout.html") {
				// Every dive has its own "dontbreak" container, so the containers must show the
				// dives one by one. Counting them would not notice a repeated and an omitted dive.
				hasContent = flowContainerTexts(html) == expectedTexts;
			} else {
				// "diveProfile" also appears in the stylesheet, so look for the id of every dive
				// and the profile image that replaces its placeholder instead.
				for (size_t i = 0; hasContent && i < divelog.dives.size(); ++i)
					hasContent = html.contains(QString("id=\"dive_%1\"").arg(divelog.dives[i]->id)) &&
						     html.contains(QString("/dive_%1.png\"").arg(divelog.dives[i]->id));
			}
		}
		dialog->close();
	});
	printer.preview();
	QVERIFY2(opened, qPrintable(fileName));
	QVERIFY2(hasContent, qPrintable(fileName));
}
#else
void TestPrinting::testPreviewDialog_data() { }
void TestPrinting::testPreviewDialog() { }
#endif

static const QString surfaceIntervalTemplate =
	"<html>\n<body data-numberofdives=\"1\">\n"
	"{% for dive in dives %}\n"
	"<div id=\"dive_{{ dive.id }}\">[{{ dive.surfaceInterval }}]</div>\n"
	"{% endfor %}\n"
	"</body>\n</html>\n";

// Extract the text rendered for {{ dive.surfaceInterval }} of the given dive
static QString renderedSurfaceInterval(const QString &html, int diveId)
{
	const QRegularExpression re(QString("<div id=\"dive_%1\">\\[([^\\]]*)\\]</div>").arg(diveId));
	const QRegularExpressionMatch match = re.match(html);
	return match.hasMatch() ? match.captured(1) : QStringLiteral("<not rendered>");
}

void TestPrinting::testSurfaceIntervalProperty()
{
	QCOMPARE(parse_file(SUBSURFACE_TEST_DATA "/dives/surface-interval.xml", &divelog), 0);
	QCOMPARE(divelog.dives.size(), 6u);

	const QString templateName = createTemplateFromString("test_print_surfint", surfaceIntervalTemplate);
	QVERIFY(!templateName.isEmpty());
	temporaryTemplates << templateName;

	const QString html = exportHtmlWithTemplate(templateName, nullptr, true);
	QVERIFY(!html.isEmpty());
	QVERIFY(!html.contains("{{ dive.surfaceInterval }}"));
	// No previous dive in the log: empty
	QCOMPARE(renderedSurfaceInterval(html, divelog.dives[0]->id), QString());
	// Second dive starts 1 h 30 min after the end of the first
	QCOMPARE(renderedSurfaceInterval(html, divelog.dives[1]->id), QString("1h 30min"));
	// Third dive starts 2 days after the end of the second
	QCOMPARE(renderedSurfaceInterval(html, divelog.dives[2]->id), QString("2d 0h 0min"));
	// Fourth dive starts 10 days after the end of the third: not capped to "more than 4 days"
	QCOMPARE(renderedSurfaceInterval(html, divelog.dives[3]->id), QString("10d 0h 0min"));
	// Fifth dive starts 59 min 40 s after the end of the fourth: rounds up to 1h 0min, not "0h 60min"
	QCOMPARE(renderedSurfaceInterval(html, divelog.dives[4]->id), QString("1h 0min"));
	// Sixth dive starts 15 min into the fifth dive: 0h 0min, not empty
	QCOMPARE(renderedSurfaceInterval(html, divelog.dives[5]->id), QString("0h 0min"));
}

void TestPrinting::testSurfaceIntervalPropertyPartialSelection()
{
	QCOMPARE(parse_file(SUBSURFACE_TEST_DATA "/dives/surface-interval.xml", &divelog), 0);
	QCOMPARE(divelog.dives.size(), 6u);

	const QString templateName = createTemplateFromString("test_print_surfint_single", surfaceIntervalTemplate);
	QVERIFY(!templateName.isEmpty());
	temporaryTemplates << templateName;

	// Printing only the second dive must still use the first dive of the log as predecessor
	const int secondId = divelog.dives[1]->id;
	const QString html = exportHtmlWithTemplate(templateName, divelog.dives[1].get());
	QVERIFY(!html.isEmpty());
	QVERIFY(!html.contains(QString("dive_%1\"").arg(divelog.dives[0]->id)));
	QVERIFY(!html.contains(QString("dive_%1\"").arg(divelog.dives[2]->id)));
	QCOMPARE(renderedSurfaceInterval(html, secondId), QString("1h 30min"));

	// Printing only the first dive still renders empty
	const QString firstHtml = exportHtmlWithTemplate(templateName, divelog.dives[0].get());
	QCOMPARE(renderedSurfaceInterval(firstHtml, divelog.dives[0]->id), QString());
}

int main(int argc, char **argv)
{
	QTemporaryDir tempHome;
	if (!tempHome.isValid())
		return 1;
	qputenv("HOME", tempHome.path().toUtf8());
	qputenv("QT_QPA_PLATFORM", QByteArray("offscreen"));
	QApplication app(argc, argv);
	TestPrinting tc;
	return QTest::qExec(&tc, argc, argv);
}
