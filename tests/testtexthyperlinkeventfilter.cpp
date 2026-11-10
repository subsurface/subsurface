// SPDX-License-Identifier: GPL-2.0
#include "testtexthyperlinkeventfilter.h"
#include "desktop-widgets/simplewidgets.h"
#include <QDesktopServices>
#include <QTextEdit>
#include <QTextCursor>

// The notes field is a QTextEdit with one text block per line. QTextCursor
// represents the boundary between two blocks as U+2029 in selectedText(),
// which must never end up in the URL that is opened.

void TestTextHyperlinkEventFilter::recordOpenedUrl(QUrl url)
{
	openedUrls.append(url);
}

void TestTextHyperlinkEventFilter::init()
{
	openedUrls.clear();
}

void TestTextHyperlinkEventFilter::ctrlClickOpensUrl_data()
{
	QTest::addColumn<QString>("text");
	QTest::addColumn<QString>("target"); // the part of the text that is clicked on
	QTest::addColumn<QString>("expected");

	const QString url = QStringLiteral("https://subsurface-divelog.org/");

	QTest::newRow("only line") << url << url << url;
	QTest::newRow("last line") << "first line\n" + url << url << url;
	QTest::newRow("end of line, followed by text") << url + "\nsecond line" << url << url;
	QTest::newRow("end of line, followed by empty line") << url + "\n\nthird line" << url << url;
	QTest::newRow("middle line") << "first line\n" + url + "\nthird line" << url << url;
	QTest::newRow("start of line, preceded by text") << "first line\n" + url + " trailing" << url << url;
	QTest::newRow("within sentence") << "see " + url + " for more\nsecond line" << url << url;
	QTest::newRow("line separator") << url + QChar(0x2028) + "second line" << url << url;
	QTest::newRow("path, no trailing slash") << "https://subsurface-divelog.org/contribute\nsecond line"
						 << "contribute" << "https://subsurface-divelog.org/contribute";
}

void TestTextHyperlinkEventFilter::ctrlClickOpensUrl()
{
	QFETCH(QString, text);
	QFETCH(QString, target);
	QFETCH(QString, expected);

	QDesktopServices::setUrlHandler(QStringLiteral("https"), this, "recordOpenedUrl");

	QTextEdit edit;
	new TextHyperlinkEventFilter(&edit); // owned by edit
	edit.setPlainText(text);
	edit.resize(600, 300);
	edit.show();
	QVERIFY(QTest::qWaitForWindowExposed(&edit));

	// Click on the middle of the target text.
	int pos = edit.toPlainText().indexOf(target) + target.size() / 2;
	QTextCursor cursor(edit.document());
	cursor.setPosition(pos);
	QTest::mouseClick(edit.viewport(), Qt::LeftButton, Qt::ControlModifier, edit.cursorRect(cursor).center());

	QDesktopServices::unsetUrlHandler(QStringLiteral("https"));

	QCOMPARE(openedUrls.size(), 1);
	QCOMPARE(openedUrls[0].toString(QUrl::FullyEncoded), expected);
}

void TestTextHyperlinkEventFilter::plainClickDoesNotOpenUrl()
{
	QDesktopServices::setUrlHandler(QStringLiteral("https"), this, "recordOpenedUrl");

	QTextEdit edit;
	new TextHyperlinkEventFilter(&edit); // owned by edit
	edit.setPlainText(QStringLiteral("https://subsurface-divelog.org/\nsecond line"));
	edit.resize(600, 300);
	edit.show();
	QVERIFY(QTest::qWaitForWindowExposed(&edit));

	QTextCursor cursor(edit.document());
	cursor.setPosition(10);
	QTest::mouseClick(edit.viewport(), Qt::LeftButton, Qt::NoModifier, edit.cursorRect(cursor).center());

	QDesktopServices::unsetUrlHandler(QStringLiteral("https"));

	QVERIFY(openedUrls.isEmpty());
}

QTEST_MAIN(TestTextHyperlinkEventFilter)
