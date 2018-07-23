// SPDX-License-Identifier: GPL-2.0
#ifndef TESTQPREFFACEBOOK_H
#define TESTQPREFFACEBOOK_H

#include <QObject>

class TestQPrefFacebook : public QObject
{
	Q_OBJECT

private slots:
	void initTestCase();
	void test_struct_get();
	void test_set_struct();
	void test_multiple();
	void test_oldPreferences();
};

#endif // TESTQPREFFACEBOOK_H
