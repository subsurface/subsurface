// SPDX-License-Identifier: GPL-2.0
#ifndef TESTTEXTHYPERLINKEVENTFILTER_H
#define TESTTEXTHYPERLINKEVENTFILTER_H

#include <QtTest>
#include <QUrl>

class TestTextHyperlinkEventFilter : public QObject {
	Q_OBJECT
public slots:
	void recordOpenedUrl(QUrl url);
private slots:
	void init();
	void ctrlClickOpensUrl_data();
	void ctrlClickOpensUrl();
	void plainClickDoesNotOpenUrl();
private:
	QList<QUrl> openedUrls;
};

#endif
