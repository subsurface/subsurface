// SPDX-License-Identifier: GPL-2.0
#ifndef TESTQML_H
#define TESTQML_H
#include <QtQuickTest>
#include <QQmlEngine>
#include <QQmlContext>
#include <QObject>

class QMLTestSetup : public QObject {
	Q_OBJECT

public:
	QMLTestSetup() {}

public slots:
	void qmlEngineAvailable(QQmlEngine *engine);
};

#endif // TESTQML_H
