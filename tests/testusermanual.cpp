// SPDX-License-Identifier: GPL-2.0
#include "testusermanual.h"

#include "desktop-widgets/usermanual.h"
#include "desktop-widgets/usermanualpath.h"
#include "desktop-widgets/mainwindow.h"
#include "core/pref.h"

#include <QApplication>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QImage>
#include <QScrollBar>
#include <QSettings>
#include <QTemporaryDir>
#include <QTest>
#include <QTextDocument>

void TestUserManual::initTestCase()
{
	Q_INIT_RESOURCE(subsurface);
	prefs = default_prefs;
#ifndef Q_OS_WIN
	// Windows does not use MainWindow::instance() in the manual's showEvent.
	// Constructing MainWindow there writes to the user's actual Roaming folder.
	prefs.update_manager.dont_check_for_updates = true; // No network or modal update prompt in a GUI test.
	// The real dialog expects MainWindow::instance() during showEvent on macOS.
	mainWindow = std::make_unique<MainWindow>();
#endif
}

void TestUserManual::cleanupTestCase()
{
	mainWindow.reset();
}

void TestUserManual::captureExternalUrl(const QUrl &url)
{
	externalUrl = url;
}

void TestUserManual::testPackagedManualSmoke()
{
	const QUrl url = getPackagedUserManualUrl();
	if (!url.isValid())
		QSKIP("Packaged manual not installed in the test data search path");
	UserManual manual(mainWindow.get());
	manual.show();
	auto *viewer = manual.findChild<QLiteHtmlWidget *>();
	QVERIFY(viewer);
	QCOMPARE(viewer->url(), url);
	QVERIFY(viewer->html().contains("S_StartUsing"));
	QVERIFY(viewer->verticalScrollBar()->maximum() > 0);
	viewer->verticalScrollBar()->setValue(viewer->verticalScrollBar()->maximum() / 2);
	QVERIFY(viewer->verticalScrollBar()->value() > 0);
	manual.resize(900, 650);
	QCoreApplication::processEvents();
	QVERIFY(viewer->verticalScrollBar()->maximum() > 0);
	QVERIFY(QMetaObject::invokeMethod(&manual, "linkClickedSlot", Q_ARG(QUrl, QUrl("#S_StartUsing"))));
	QVERIFY(viewer->verticalScrollBar()->value() > 0);
	QVERIFY(QMetaObject::invokeMethod(&manual, "searchTextChanged", Q_ARG(QString, QString("Subsurface"))));
	QCOMPARE(viewer->selectedText().toCaseFolded(), QString("subsurface"));
}

void TestUserManual::testLocalResourcesAnchorsAndSearch()
{
	QTemporaryDir dir;
	QVERIFY(dir.isValid());
	QFile css(dir.filePath("fixture.css"));
	QVERIFY(css.open(QIODevice::WriteOnly));
	QVERIFY(css.write(".fixture { color: rgb(0, 200, 0); }\n") > 0);
	css.close();
	QImage icon(20, 20, QImage::Format_RGB32);
	icon.fill(Qt::red);
	QVERIFY(icon.save(dir.filePath("fixture.png")));

	// Use the real dialog and its resource handler, search slots and link-delegation slot.
	UserManual manual(mainWindow.get());
	manual.show();
	auto *viewer = manual.findChild<QLiteHtmlWidget *>();
	QVERIFY(viewer);
	viewer->setUrl(QUrl::fromLocalFile(dir.filePath("fixture.html")));
	viewer->setHtml(QString::fromUtf8(
		"<html><head><link rel='stylesheet' href='fixture.css'></head><body>"
		"<p class='fixture'>Kia ora, ruku hōhonu</p><img src='fixture.png'>"
		"<a href='#target'>Go to target</a><div style='height:1600px'></div>"
		"<p id='target'>needle one</p><div style='height:700px'></div>"
		"<p>needle two</p></body></html>"));
	QCoreApplication::processEvents();
	QVERIFY(viewer->verticalScrollBar()->maximum() > 0);
	QCOMPARE(viewer->html().contains(QString::fromUtf8("ruku hōhonu")), true);
	QImage rendered(viewer->viewport()->size(), QImage::Format_RGB32);
	rendered.fill(Qt::white);
	viewer->viewport()->render(&rendered);
	bool imageLoaded = false;
	bool cssLoaded = false;
	for (int y = 0; y < rendered.height(); ++y) {
		for (int x = 0; x < rendered.width(); ++x) {
			const QColor color = rendered.pixelColor(x, y);
			imageLoaded |= color.red() > 200 && color.green() < 40 && color.blue() < 40;
			cssLoaded |= color.green() > 100 && color.red() < 80 && color.blue() < 80;
		}
	}
	QVERIFY(imageLoaded);
	QVERIFY(cssLoaded);

	QVERIFY(QMetaObject::invokeMethod(&manual, "linkClickedSlot", Q_ARG(QUrl, QUrl("#target"))));
	QVERIFY(viewer->verticalScrollBar()->value() > 0);
	QVERIFY(QMetaObject::invokeMethod(&manual, "searchTextChanged", Q_ARG(QString, QString("needle"))));
	QCOMPARE(viewer->selectedText(), QString("needle"));
	const int firstMatchY = viewer->verticalScrollBar()->value();
	QVERIFY(QMetaObject::invokeMethod(&manual, "searchNext"));
	QCOMPARE(viewer->selectedText(), QString("needle"));
	const int secondMatchY = viewer->verticalScrollBar()->value();
	QVERIFY(secondMatchY > firstMatchY);
	QVERIFY(QMetaObject::invokeMethod(&manual, "searchPrev"));
	QCOMPARE(viewer->selectedText(), QString("needle"));
	QVERIFY(viewer->verticalScrollBar()->value() < secondMatchY);
	const QUrl external("https://example.org/manual");
	QDesktopServices::setUrlHandler("https", this, "captureExternalUrl");
	QVERIFY(QMetaObject::invokeMethod(&manual, "linkClickedSlot", Q_ARG(QUrl, external)));
	QDesktopServices::unsetUrlHandler("https");
	QCOMPARE(externalUrl, external);
	viewer->scrollToAnchor(QString());
	QCOMPARE(viewer->verticalScrollBar()->value(), 0);
}

int main(int argc, char **argv)
{
	QTemporaryDir home;
	if (!home.isValid())
		return 1;
	qputenv("HOME", home.path().toUtf8());
	qputenv("XDG_CONFIG_HOME", home.path().toUtf8());
	qputenv("APPDATA", home.path().toUtf8());
	QSettings::setDefaultFormat(QSettings::IniFormat);
	QApplication app(argc, argv);
	// Without a bundled printing_templates directory, MainWindow's template copy starts from an
	// empty path, which QDir resolves to the current directory, and creates a directory in the
	// user's template folder for each entry it finds there. Run from the empty temporary
	// directory so that it finds nothing, and so that the test needs no files beside the executable.
	// Qt resolves a relative argv[0] against the current directory when /proc/self/exe is not
	// available, so look up and cache the executable path before changing directory.
	QCoreApplication::applicationFilePath();
	if (!QDir::setCurrent(home.path()))
		return 1;
	TestUserManual tc;
	return QTest::qExec(&tc, argc, argv);
}
