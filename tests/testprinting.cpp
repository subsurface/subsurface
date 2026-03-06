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
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QImage>
#include <QTextStream>

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
	const QString templateName = QString("%1_%2.html").arg(prefix).arg(QDateTime::currentMSecsSinceEpoch());
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

QString TestPrinting::exportHtmlWithTemplate(const QString &templateName) const
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

	QImage image(1200, 800, QImage::Format_ARGB32);
	image.fill(Qt::white);

	if (divelog.dives.empty())
		return QString();
	Printer printer(&image, printOptions, templateOptions, Printer::PREVIEW, divelog.dives[0].get());
	return printer.exportHtml();
}

QImage TestPrinting::renderWithTemplate(const QString &templateName) const
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
		return 0;
	int differences = 0;
	for (int y = 0; y < a.height(); ++y) {
		for (int x = 0; x < a.width(); ++x) {
			if (a.pixelColor(x, y) != b.pixelColor(x, y))
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
	QVERIFY2(changedPixels > 200, qPrintable(QString("%1 changedPixels=%2").arg(templatePath).arg(changedPixels)));
}

int main(int argc, char **argv)
{
	qputenv("QT_QPA_PLATFORM", QByteArray("offscreen"));
	QApplication app(argc, argv);
	TestPrinting tc;
	return QTest::qExec(&tc, argc, argv);
}
