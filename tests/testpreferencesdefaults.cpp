// SPDX-License-Identifier: GPL-2.0
#include "testpreferencesdefaults.h"

#include "core/pref.h"
#include "core/settings/qPrefDisplay.h"
#include "desktop-widgets/preferences/preferences_defaults.h"

#include <QApplication>
#include <QComboBox>
#include <QPalette>
#include <QSettings>

void TestPreferencesDefaults::initTestCase()
{
	TestBase::initTestCase();

	QCoreApplication::setOrganizationName("Subsurface");
	QCoreApplication::setOrganizationDomain("subsurface.hohndel.org");
	QCoreApplication::setApplicationName("SubsurfaceTestPreferencesDefaults");

	prefs.divelist_font = "Sans";
}

#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
static QComboBox *colorSchemeCombo(PreferencesDefaults &widget)
{
	return widget.findChild<QComboBox *>(QStringLiteral("colorScheme"));
}

// The Appearance drop-down must offer the three choices, each carrying the
// stored value, so that a saved preference can be selected again.
void TestPreferencesDefaults::test_color_scheme_items()
{
	PreferencesDefaults widget;
	QComboBox *combo = colorSchemeCombo(widget);
	QVERIFY(combo);
	QCOMPARE(combo->count(), 3);
	QCOMPARE(combo->itemData(0).toString(), QStringLiteral("system"));
	QCOMPARE(combo->itemData(1).toString(), QStringLiteral("light"));
	QCOMPARE(combo->itemData(2).toString(), QStringLiteral("dark"));
	for (int i = 0; i < combo->count(); i++)
		QVERIFY(!combo->itemText(i).isEmpty());
}

// Select, save, "restart" (reload the saved settings and open the dialog again)
// and check that the drop-down shows the saved choice instead of being empty.
void TestPreferencesDefaults::test_color_scheme_refresh_and_sync()
{
	const QString key = QStringLiteral("Display/color_scheme");
	QSettings().remove(key);
	qPrefDisplay::load();

	{
		PreferencesDefaults widget;
		QComboBox *combo = colorSchemeCombo(widget);
		QVERIFY(combo);

		widget.refreshSettings();
		QCOMPARE(combo->currentData().toString(), QStringLiteral("system"));
		QVERIFY(!combo->currentText().isEmpty());

		combo->setCurrentIndex(combo->findData(QStringLiteral("dark")));
		widget.syncSettings();
		QCOMPARE(qPrefDisplay::color_scheme(), QStringLiteral("dark"));
		QCOMPARE(QSettings().value(key).toString(), QStringLiteral("dark"));
	}

	// Simulate a restart: forget the in-memory value and reload from disk
	qPrefDisplay::set_color_scheme(QStringLiteral("system"));
	QSettings().setValue(key, QStringLiteral("dark"));
	qPrefDisplay::load();

	for (const char *value : { "dark", "light", "system" }) {
		QSettings().setValue(key, QString::fromLatin1(value));
		qPrefDisplay::load();
		PreferencesDefaults widget;
		QComboBox *combo = colorSchemeCombo(widget);
		widget.refreshSettings();
		QVERIFY2(combo->currentIndex() >= 0, value);
		QCOMPARE(combo->currentData().toString(), QString::fromLatin1(value));
		QVERIFY(!combo->currentText().isEmpty());
	}

	qPrefDisplay::set_color_scheme(QStringLiteral("system"));
}

// Apply and Save both end up in syncSettings(): the choice has to change the
// appearance of the running application, and Use system has to undo it.
void TestPreferencesDefaults::test_color_scheme_applies()
{
	QVERIFY2(qobject_cast<QApplication *>(QCoreApplication::instance()), "needs a widgets application");
	QSettings().remove(QStringLiteral("Display/color_scheme"));
	qPrefDisplay::load();
	const QPalette systemPalette = QApplication::palette();

	PreferencesDefaults widget;
	QComboBox *combo = colorSchemeCombo(widget);
	QVERIFY(combo);

	combo->setCurrentIndex(combo->findData(QStringLiteral("dark")));
	widget.syncSettings();
	QVERIFY(QApplication::palette().color(QPalette::Window).lightnessF() < 0.5);
	QVERIFY(QApplication::palette().color(QPalette::WindowText).lightnessF() > 0.5);

	combo->setCurrentIndex(combo->findData(QStringLiteral("light")));
	widget.syncSettings();
	QVERIFY(QApplication::palette().color(QPalette::Window).lightnessF() > 0.5);
	QVERIFY(QApplication::palette().color(QPalette::WindowText).lightnessF() < 0.5);

	combo->setCurrentIndex(combo->findData(QStringLiteral("system")));
	widget.syncSettings();
	QCOMPARE(QApplication::palette(), systemPalette);
}

// The selector must stay usable whatever the platform theme does with the
// request: Qt may ignore it, but that is not for the preference page to decide.
void TestPreferencesDefaults::test_color_scheme_always_selectable()
{
	PreferencesDefaults widget;
	QComboBox *combo = colorSchemeCombo(widget);
	QVERIFY(combo);
	QVERIFY(combo->isEnabled());
	for (int i = 0; i < combo->count(); i++) {
		QVERIFY(combo->model()->flags(combo->model()->index(i, 0)) & Qt::ItemIsEnabled);
		combo->setCurrentIndex(i);
		QCOMPARE(combo->currentIndex(), i);
	}
}
#endif

QTEST_MAIN(TestPreferencesDefaults)
