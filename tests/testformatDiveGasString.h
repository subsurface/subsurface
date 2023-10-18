// SPDX-License-Identifier: GPL-2.0

#include <QtTest>

class TestformatDiveGasString : public QObject {
	Q_OBJECT
private slots:
	void init();
	void test_empty();
	void test_air();
	void test_nitrox();
	void test_nitrox_not_use();
	void test_nitrox_deco();
	void test_reverse_nitrox_deco();
	void test_trimix();
	void test_trimix_deco();
	void test_reverse_trimix_deco();
	void test_trimix_and_nitrox_same_o2();
	void test_trimix_and_nitrox_lower_o2();
	void test_ccr();
	void test_ccr_bailout();
};
