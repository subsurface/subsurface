// SPDX-License-Identifier: GPL-2.0
#ifndef TESTUSERMANUAL_H
#define TESTUSERMANUAL_H

#include "desktop-widgets/mainwindow.h"

#include <QObject>
#include <QUrl>
#include <memory>

class TestUserManual : public QObject {
	Q_OBJECT
private slots:
	void initTestCase();
	void cleanupTestCase();
	void testLocalResourcesAnchorsAndSearch();
	void testPackagedManualSmoke();
	void captureExternalUrl(const QUrl &url);
private:
	std::unique_ptr<MainWindow> mainWindow;
	QUrl externalUrl;
};

#endif
