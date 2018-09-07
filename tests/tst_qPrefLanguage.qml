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
}
