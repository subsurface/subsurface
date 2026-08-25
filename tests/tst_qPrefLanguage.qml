// SPDX-License-Identifier: GPL-2.0
import QtQuick 2.6
import QtTest 1.2
TestCase {
	name: "qPrefLanguage"
	property date plannerDateTime: new Date(2026, 7, 26, 13, 5, 0)
	property string plannerDateDisplay: Qt.formatDate(plannerDateTime, PrefLanguage.date_format_short)
	property string plannerTimeDisplay: Qt.formatTime(plannerDateTime, PrefLanguage.time_format)

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

		PrefLanguage.applyDateTimeFormats("yyyy/MM/dd", "yy/M/d", "HH:mm", true, true)
		compare(PrefLanguage.effectiveDateFormat, "yyyy/MM/dd")
		compare(PrefLanguage.effectiveDateFormatShort, "yy/M/d")
		compare(PrefLanguage.effectiveTimeFormat, "HH:mm")
		verify(PrefLanguage.longDatePreview.length > 0)
		verify(PrefLanguage.shortDatePreview.length > 0)
		verify(PrefLanguage.timePreview.length > 0)
		compare(PrefLanguage.dateFormatPresets.length, 4)
		compare(PrefLanguage.timeFormatPresets.length, 3)
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
		property int formatRefreshCount: 0

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
			function onDateTimeFormatsChanged() { ++spyCatcher.formatRefreshCount }
		}
	}

	// AI-generated (Claude): Exercise the mobile preset choices through the public QML API.
	function test_mobilePresets() {
		PrefLanguage.use_system_language = false
		PrefLanguage.lang_locale = "en_US"
		PrefLanguage.applyDatePreset("system")

		var refreshCount = spyCatcher.formatRefreshCount
		PrefLanguage.applyDatePreset("day-first")
		compare(PrefLanguage.date_format, "dd.MM.yyyy")
		compare(PrefLanguage.date_format_short, "d.M.yy")
		compare(PrefLanguage.date_format_override, true)
		compare(spyCatcher.formatRefreshCount, refreshCount + 1)

		PrefLanguage.applyDatePreset("iso")
		compare(PrefLanguage.date_format, "yyyy-MM-dd")
		compare(PrefLanguage.date_format_short, "yy-M-d")
		PrefLanguage.applyDatePreset("month-first")
		compare(PrefLanguage.date_format, "MM/dd/yyyy")
		compare(PrefLanguage.date_format_short, "M/d/yy")

		PrefLanguage.applyTimePreset("24-hour")
		compare(PrefLanguage.time_format, "hh:mm")
		compare(PrefLanguage.time_format_override, true)
		PrefLanguage.applyTimePreset("12-hour")
		compare(PrefLanguage.time_format, "h:mm AP")

		PrefLanguage.applyDatePreset("system")
		PrefLanguage.applyTimePreset("system")
		compare(PrefLanguage.date_format_override, false)
		compare(PrefLanguage.time_format_override, false)
		verify(PrefLanguage.date_format.length > 0)
		verify(PrefLanguage.date_format_short.length > 0)
		verify(PrefLanguage.time_format.length > 0)
	}

	// AI-generated (Claude): The planner binds to these preferences while it remains instantiated.
	function test_plannerDateTimeDisplayFormats() {
		PrefLanguage.use_system_language = false
		PrefLanguage.lang_locale = "en_US"

		PrefLanguage.applyDatePreset("day-first")
		compare(plannerDateDisplay, "26.8.26")
		PrefLanguage.applyDatePreset("month-first")
		compare(plannerDateDisplay, "8/26/26")
		PrefLanguage.applyDatePreset("iso")
		compare(plannerDateDisplay, "26-8-26")
		PrefLanguage.applyDateTimeFormats("yyyy-MM-dd", "yyyy-MM-dd", "HH:mm", true, true)
		compare(plannerDateDisplay, "2026-08-26")
		compare(plannerTimeDisplay, "13:05")

		PrefLanguage.applyTimePreset("12-hour")
		compare(plannerTimeDisplay, "1:05 PM")
		// The time input fields drop the digits-only IME hint when the format
		// carries a meridiem token, so AM/PM letters stay reachable on Android.
		verify(/AP|ap/.test(PrefLanguage.time_format))
		PrefLanguage.applyTimePreset("24-hour")
		compare(plannerTimeDisplay, "13:05")
		verify(!/AP|ap/.test(PrefLanguage.time_format))
	}

	// AI-generated (Claude): Exercise keypad conversion, meridiem changes and safe rejection.
	function test_mobileDateTimeKeypadRoundTrips() {
		PrefLanguage.use_system_language = false
		PrefLanguage.lang_locale = "en_US"
		PrefLanguage.applyDateTimeFormats("MM/dd/yyyy", "M/d/yy", "h:mm AP", true, true)

		compare(PrefLanguage.timeEditText("1:05 PM"), "1:05 PM")
		compare(PrefLanguage.toggleMeridiem("1:05 PM", false), "1:05 AM")
		compare(PrefLanguage.timeDisplayText("1:05 AM"), "1:05 AM")
		compare(PrefLanguage.dateTimeEditText("8/26/26 1:05 PM"), "8/26/26 1:05 PM")
		compare(PrefLanguage.toggleMeridiem("8/26/26 1:05 PM", true), "8/26/26 1:05 AM")
		compare(PrefLanguage.dateTimeDisplayText("8/26/26 1:05 AM"), "8/26/26 1:05 AM")

		PrefLanguage.applyDateTimeFormats("dd.MM.yyyy", "d.M.yy", "HH.mm", true, true)
		compare(PrefLanguage.dateTimeEditText("26.8.26 13.05"), "26/8/26 13:05")
		compare(PrefLanguage.dateTimeDisplayText("26/8/26 13:05"), "26.8.26 13.05")
		compare(PrefLanguage.dateTimeDisplayText("31/2/26 13:05"), "")
		compare(PrefLanguage.toggleMeridiem("incomplete", false), "incomplete")
	}

	// AI-generated (Claude): Reading preset data must not replace persisted custom formats.
	function test_customFormatsRemainUntouched() {
		PrefLanguage.applyDateTimeFormats("yyyy/MM/dd", "yy/M/d", "HH.mm", true, true)

		compare(PrefLanguage.dateFormatPresets.length, 4)
		compare(PrefLanguage.timeFormatPresets.length, 3)
		verify(PrefLanguage.longDatePreview.length > 0)
		verify(PrefLanguage.shortDatePreview.length > 0)
		verify(PrefLanguage.timePreview.length > 0)
		compare(PrefLanguage.date_format, "yyyy/MM/dd")
		compare(PrefLanguage.date_format_short, "yy/M/d")
		compare(PrefLanguage.time_format, "HH.mm")
		compare(PrefLanguage.date_format_override, true)
		compare(PrefLanguage.time_format_override, true)
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
