// SPDX-License-Identifier: GPL-2.0
import QtQuick 2.6
import QtTest 1.2

TestCase {
	name: "qPrefGeneral"

	function test_variables() {
		var x1 = PrefGeneral.auto_recalculate_thumbnails
		PrefGeneral.auto_recalculate_thumbnails = true
		compare(PrefGeneral.auto_recalculate_thumbnails, true)

		var x2 = PrefGeneral.default_cylinder
		PrefGeneral.default_cylinder = "my string"
		compare(PrefGeneral.default_cylinder, "my string")

		var x3 = PrefGeneral.default_filename
		PrefGeneral.default_filename = "my string"
		compare(PrefGeneral.default_filename, "my string")

//TBD		var x4 = PrefGeneral.default_file_behavior
//TBD		PrefGeneral.default_file_behavior = ??
//TBD		compare(PrefGeneral.default_file_behavior, ??)

		var x5 = PrefGeneral.defaultsetpoint
		PrefGeneral.defaultsetpoint = 17
		compare(PrefGeneral.defaultsetpoint, 17)

		var x6 = PrefGeneral.extract_video_thumbnails
		PrefGeneral.extract_video_thumbnails = true
		compare(PrefGeneral.extract_video_thumbnails, true)

		var x7 = PrefGeneral.extract_video_thumbnails_position
		PrefGeneral.extract_video_thumbnails_position = 17
		compare(PrefGeneral.extract_video_thumbnails_position, 17)

		var x8 = PrefGeneral.ffmpeg_executable
		PrefGeneral.ffmpeg_executable = "my string"
		compare(PrefGeneral.ffmpeg_executable, "my string")

		var x9 = PrefGeneral.o2consumption
		PrefGeneral.o2consumption = 17
		compare(PrefGeneral.o2consumption, 17)

		var x10 = PrefGeneral.pscr_ratio
		PrefGeneral.pscr_ratio = 17
		compare(PrefGeneral.pscr_ratio, 17)

		var x11 = PrefGeneral.use_default_file
		PrefGeneral.use_default_file = true
		compare(PrefGeneral.use_default_file, true)

		var x12 = PrefGeneral.diveshareExport_uid
		PrefGeneral.diveshareExport_uid = "myUid"
		compare(PrefGeneral.diveshareExport_uid, "myUid")

		var x13 = PrefGeneral.diveshareExport_private
		PrefGeneral.diveshareExport_private = true
		compare(PrefGeneral.diveshareExport_private, true)
	}

	Item {
		id: spyCatcher

		property bool spy1 : false
		property bool spy2 : false
		property bool spy3 : false
		property bool spy5 : false
		property bool spy6 : false
		property bool spy7 : false
		property bool spy8 : false
		property bool spy9 : false
		property bool spy10 : false
		property bool spy11 : false
		property bool spy12 : false
		property bool spy13 : false

		Connections {
			target: PrefGeneral
			onAuto_recalculate_thumbnailsChanged: {spyCatcher.spy1 = true }
			onDefault_cylinderChanged: {spyCatcher.spy2 = true }
			onDefault_filenameChanged: {spyCatcher.spy3 = true }
			onDefaultsetpointChanged: {spyCatcher.spy5 = true }
			onExtract_video_thumbnailsChanged: {spyCatcher.spy6 = true }
			onExtract_video_thumbnails_positionChanged: {spyCatcher.spy7 = true }
			onFfmpeg_executableChanged: {spyCatcher.spy8 = true }
			onO2consumptionChanged: {spyCatcher.spy9 = true }
			onPscr_ratioChanged: {spyCatcher.spy10 = true }
			onUse_default_fileChanged: {spyCatcher.spy11 = true }
			onDiveshareExport_uidChanged: {spyCatcher.spy12 = true }
			onDiveshareExport_privateChanged: {spyCatcher.spy13 = true }
		}
	}

	function test_signals() {
		PrefGeneral.auto_recalculate_thumbnails = ! PrefGeneral.auto_recalculate_thumbnails
		PrefGeneral.default_cylinder = "qml"
		PrefGeneral.default_filename = "qml"
		// 4 is not emitting signals
		PrefGeneral.defaultsetpoint = -17
		PrefGeneral.extract_video_thumbnails = ! PrefGeneral.extract_video_thumbnails
		PrefGeneral.extract_video_thumbnails_position = -17
		PrefGeneral.ffmpeg_executable = "qml"
		PrefGeneral.o2consumption = -17
		PrefGeneral.pscr_ratio = -17
		PrefGeneral.use_default_file = ! PrefGeneral.use_default_file
		PrefGeneral.diveshareExport_uid = "qml"
		PrefGeneral.diveshareExport_private = ! PrefGeneral.diveshareExport_private

		compare(spyCatcher.spy1, true)
		compare(spyCatcher.spy2, true)
		compare(spyCatcher.spy3, true)
		compare(spyCatcher.spy5, true)
		compare(spyCatcher.spy6, true)
		compare(spyCatcher.spy7, true)
		compare(spyCatcher.spy8, true)
		compare(spyCatcher.spy9, true)
		compare(spyCatcher.spy10, true)
		compare(spyCatcher.spy11, true)
		compare(spyCatcher.spy12, true)
		compare(spyCatcher.spy13, true)
	}
}
