// SPDX-License-Identifier: GPL-2.0
// AI-generated (Claude)
#include "testformatnotes.h"

#include "../core/dive.h"
#include "../core/divecomputer.h"
#include "../core/string-format.h"

#include <QTextDocument>

// This is the conversion applied to the rich text coming from the mobile
// notes editor (see QMLManager::commitChanges()).
static QString richTextToPlainText(const QString &html)
{
	QTextDocument doc;
	doc.setHtml(html);
	return doc.toPlainText();
}

// Simulate opening the notes in the editor and saving them unchanged: the
// editor serialises its document back to HTML before it is converted.
static QString editorRoundTrip(const QString &html)
{
	QTextDocument editor;
	editor.setHtml(html);
	return richTextToPlainText(editor.toHtml());
}

static dive diveWithNotes(const char *notes, bool planner = false)
{
	dive d;
	d.notes = notes;
	if (planner)
		make_planner_dc(&d.dcs[0]); // A dive always has at least one dive computer
	return d;
}

void TestFormatNotes::test_empty()
{
	dive d = diveWithNotes("");
	QCOMPARE(formatNotes(&d), QString());
	QCOMPARE(richTextToPlainText(formatNotes(&d)), QString());
}

void TestFormatNotes::test_angle_brackets()
{
	dive d = diveWithNotes("<5 min");
	QCOMPARE(formatNotes(&d), QString("&lt;5 min"));
	QCOMPARE(richTextToPlainText(formatNotes(&d)), QString("<5 min"));

	d = diveWithNotes("a & b <c>");
	QCOMPARE(formatNotes(&d), QString("a &amp; b &lt;c&gt;"));
	QCOMPARE(richTextToPlainText(formatNotes(&d)), QString("a & b <c>"));
}

void TestFormatNotes::test_ampersands()
{
	dive d = diveWithNotes("a & b");
	QCOMPARE(formatNotes(&d), QString("a &amp; b"));
	QCOMPARE(richTextToPlainText(formatNotes(&d)), QString("a & b"));
}

void TestFormatNotes::test_literal_entity()
{
	// An entity typed literally by the user must not be decoded on display
	dive d = diveWithNotes("&lt; &amp; &nbsp;");
	QCOMPARE(formatNotes(&d), QString("&amp;lt; &amp;amp; &amp;nbsp;"));
	QCOMPARE(richTextToPlainText(formatNotes(&d)), QString("&lt; &amp; &nbsp;"));
}

void TestFormatNotes::test_multiline()
{
	dive d = diveWithNotes("first\nsecond\n\nfourth");
	QCOMPARE(formatNotes(&d), QString("first<br/>second<br/><br/>fourth"));
	QCOMPARE(richTextToPlainText(formatNotes(&d)), QString("first\nsecond\n\nfourth"));
}

void TestFormatNotes::test_special_characters_multiline_round_trip()
{
	// The unchanged edit/save round trip must reproduce the notes exactly,
	// so that QMLManager::commitChanges() sees no change.
	const char *notes = "visibility <5 m & current >2 kn\nsecond line: &lt;b&gt;not bold&lt;/b&gt;\n<b>also not bold</b>";
	dive d = diveWithNotes(notes);
	QCOMPARE(richTextToPlainText(formatNotes(&d)), QString(notes));
	QCOMPARE(editorRoundTrip(formatNotes(&d)), QString(notes));
}

// Planner notes are stored as HTML. Their line breaks must survive and the
// text must not be interpreted a second time as HTML. The expected string is
// the output from before the notes were escaped: text with no special
// characters must not change.
void TestFormatNotes::test_planner_notes()
{
	dive d = diveWithNotes("<div>\nline one<br/>\nline two&nbsp;&mdash; end<br/>\n</div>\n"
			       "<table>\n<thead>\n<tr><th>a</th><th>b</th></tr>\n</thead>\n"
			       "<tbody>\n<tr><td>1</td><td>2</td></tr>\n</tbody>\n</table>\n", true);
	const QString formatted = formatNotes(&d);
	QCOMPARE(formatted, QString::fromUtf8("line one\n<br/> line two \u2014 end\n<br/> <br/> \na\nb\n<br/> \n1\n2\n<br/> \n"));
	// What the user sees must be the plain-text planner notes, broken into lines
	const QString shown = richTextToPlainText(formatted);
	// An unchanged save must compare equal, so that planner notes are not overwritten
	QCOMPARE(editorRoundTrip(formatted), shown);
	QCOMPARE(shown, QString::fromUtf8("line one \nline two \u2014 end \n\na b \n1 2 \n"));
}

void TestFormatNotes::test_planner_escapes_literal_angle_bracket()
{
	// "&lt;" and "&amp;" in the HTML are the planner's encoding of '<' and '&'
	dive d = diveWithNotes("<div>\nend pressure &lt; 50 bar &amp; more<br/>\nsecond line<br/>\n</div>\n", true);
	QString formatted = formatNotes(&d);
	QCOMPARE(formatted, QString("end pressure &lt; 50 bar &amp; more\n<br/> second line\n<br/> "));
	const QString shown = richTextToPlainText(formatted);
	QCOMPARE(editorRoundTrip(formatted), shown);
	QCOMPARE(shown, QString("end pressure < 50 bar & more \nsecond line \n"));
}

QTEST_GUILESS_MAIN(TestFormatNotes)
