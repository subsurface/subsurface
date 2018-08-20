// SPDX-License-Identifier: GPL-2.0
import QtQuick 2.6
import QtTest 1.2
import org.subsurfacedivelog.mobile 1.0

TestCase {
	name: "qPrefGeneral"

	SsrfGeneralPrefs {
		id: tst
	}

	SsrfPrefs {
		id: prefs
	}

	function test_variables() {
		var x1 = tst.auto_recalculate_thumbnails
		tst.auto_recalculate_thumbnails = true
		compare(tst.auto_recalculate_thumbnails, true)

		var x2 = tst.default_cylinder
		tst.default_cylinder = "my string"
		compare(tst.default_cylinder, "my string")

		var x3 = tst.default_filename
		tst.default_filename = "my string"
		compare(tst.default_filename, "my string")

//TBD		var x4 = tst.default_file_behavior
//TBD		tst.default_file_behavior = ??
//TBD		compare(tst.default_file_behavior, ??)

		var x5 = tst.defaultsetpoint
		tst.defaultsetpoint = 17
		compare(tst.defaultsetpoint, 17)

		var x6 = tst.extract_video_thumbnails
		tst.extract_video_thumbnails = true
		compare(tst.extract_video_thumbnails, true)

		var x7 = tst.extract_video_thumbnails_position
		tst.extract_video_thumbnails_position = 17
		compare(tst.extract_video_thumbnails_position, 17)

		var x8 = tst.ffmpeg_executable
		tst.ffmpeg_executable = "my string"
		compare(tst.ffmpeg_executable, "my string")

		var x9 = tst.o2consumption
		tst.o2consumption = 17
		compare(tst.o2consumption, 17)

		var x10 = tst.pscr_ratio
		tst.pscr_ratio = 17
		compare(tst.pscr_ratio, 17)

		var x11 = tst.use_default_file
		tst.use_default_file = true
		compare(tst.use_default_file, true)

		var x12 = tst.diveshareExport_uid
		tst.diveshareExport_uid = "myUid"
		compare(tst.diveshareExport_uid, "myUid")

		var x13 = tst.diveshareExport_private
		tst.diveshareExport_private = true
		compare(tst.diveshareExport_private, true)
	}
}
