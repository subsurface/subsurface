// SPDX-License-Identifier: GPL-2.0
#ifndef TESTPICTURE_H
#define TESTPICTURE_H

#include <QtTest>

class TestPicture : public QObject{
	Q_OBJECT
private slots:
	void initTestCase();
	void addPicture();
};

#endif
