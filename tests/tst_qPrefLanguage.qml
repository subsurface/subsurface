// SPDX-License-Identifier: GPL-2.0
import QtQuick 2.6
import QtTest 1.2
TestCase {
	name: "qPrefLanguage"

	function test_variables() {
		var x1 = PrefLanguage.date_format
		PrefLanguage.date_format = "new date format"
		compare(PrefLanguage.date_format, "new date format")

		var x2 = PrefLanguage.date_format_override
		PrefLanguage.date_format_override = true
		compare(PrefLanguage.date_format_override, true)

		var x3 = PrefLanguage.date_format_short
		PrefLanguage.date_format_short = "new short format"
		compare(PrefLanguage.date_format_short, "new short format")

		var x4 = PrefLanguage.language
		PrefLanguage.language = "new lang format"
		compare(PrefLanguage.language, "new lang format")

		var x5 = PrefLanguage.lang_locale
		PrefLanguage.lang_locale = "new loc lang format"
		compare(PrefLanguage.lang_locale, "new loc lang format")

		var x6 = PrefLanguage.time_format
		PrefLanguage.time_format = "new time format"
		compare(PrefLanguage.time_format, "new time format")

		var x7 = PrefLanguage.time_format_override
		PrefLanguage.time_format_override = true
		compare(PrefLanguage.time_format_override, true)

		var x8 = PrefLanguage.use_system_language
		PrefLanguage.use_system_language = true
		compare(PrefLanguage.use_system_language, true)
	}

	Item {
		id: spyCatcher

		property bool spy1 : false
		property bool spy2 : false
		property bool spy3 : false
		property bool spy4 : false
		property bool spy5 : false
		property bool spy6 : false
		property bool spy7 : false
		property bool spy8 : false

		Connections {
			target: PrefLanguage
			onDate_formatChanged: {spyCatcher.spy1 = true }
			onDate_format_overrideChanged: {spyCatcher.spy2 = true }
			onDate_format_shortChanged: {spyCatcher.spy3 = true }
			onLanguageChanged: {spyCatcher.spy4 = true }
			onLang_localeChanged: {spyCatcher.spy5 = true }
			onTime_formatChanged: {spyCatcher.spy6 = true }
			onTime_format_overrideChanged: {spyCatcher.spy7 = true }
			onUse_system_languageChanged: {spyCatcher.spy8 = true }
		}
	}

	function test_signals() {
		PrefLanguage.date_format = "qml"
		PrefLanguage.date_format_override = ! PrefLanguage.date_format_override
		PrefLanguage.date_format_short = "qml"
		PrefLanguage.language = "qml"
		PrefLanguage.lang_locale = "qml"
		PrefLanguage.time_format = "qml"
		PrefLanguage.time_format_override = ! PrefLanguage.time_format_override
		PrefLanguage.use_system_language = ! PrefLanguage.use_system_language

		compare(spyCatcher.spy1, true)
		compare(spyCatcher.spy2, true)
		compare(spyCatcher.spy3, true)
		compare(spyCatcher.spy4, true)
		compare(spyCatcher.spy5, true)
		compare(spyCatcher.spy6, true)
		compare(spyCatcher.spy7, true)
		compare(spyCatcher.spy8, true)
	}
}
