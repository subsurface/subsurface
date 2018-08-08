// SPDX-License-Identifier: GPL-2.0
import QtQuick 2.6
import QtTest 1.2
import org.subsurfacedivelog.mobile 1.0

TestCase {
	name: "qPrefLanguage"

	SsrfLanguagePrefs {
		id: tst
	}

	SsrfPrefs {
		id: prefs
	}

	function test_variables() {
		var x1 = tst.date_format
		tst.date_format = "new date format"
		compare(tst.date_format, "new date format")

		var x2 = tst.date_format_override
		tst.date_format_override = true
		compare(tst.date_format_override, true)

		var x3 = tst.date_format_short
		tst.date_format_short = "new short format"
		compare(tst.date_format_short, "new short format")

		var x4 = tst.language
		tst.language = "new lang format"
		compare(tst.language, "new lang format")

		var x5 = tst.lang_locale
		tst.lang_locale = "new loc lang format"
		compare(tst.lang_locale, "new loc lang format")

		var x6 = tst.time_format
		tst.time_format = "new time format"
		compare(tst.time_format, "new time format")

		var x7 = tst.time_format_override
		tst.time_format_override = true
		compare(tst.time_format_override, true)

		var x8 = tst.use_system_language
		tst.use_system_language = true
		compare(tst.use_system_language, true)
	}
}
