// SPDX-License-Identifier: GPL-2.0
#ifndef TESTPRINTING_H
#define TESTPRINTING_H

#include "testbase.h"

#include <QImage>
#include <QStringList>

class TestPrinting : public TestBase {
	Q_OBJECT
private slots:
	void initTestCase();
	void cleanup();
	void testPrinterExportHtmlDiveId_data();
	void testPrinterExportHtmlDiveId();
	void testPrinterRenderRecognizesDiveProfile_data();
	void testPrinterRenderRecognizesDiveProfile();

private:
	QString createTemplateFromFile(const QString &prefix, const QString &sourcePath) const;
	QString createTemplateFromString(const QString &prefix, const QString &contents) const;
	QImage renderWithTemplate(const QString &templateName) const;
	QString exportHtmlWithTemplate(const QString &templateName) const;
	void removeTemplate(const QString &templateName) const;
	QString loadFileContents(const QString &path) const;
	int imageDifferencePixels(const QImage &a, const QImage &b) const;
	QStringList temporaryTemplates;
};

#endif