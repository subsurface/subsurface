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
}
