// SPDX-License-Identifier: GPL-2.0
import QtQuick 2.6
import QtTest 1.2
import org.subsurfacedivelog.mobile 1.0

TestCase {
	name: "qPrefDiveComputer"

	SsrfDiveComputerPrefs {
		id: tst
	}

	SsrfPrefs {
		id: prefs
	}

	function test_variables() {
		var x1 = tst.device
		tst.device = "my device"
		compare(tst.device, "my device")

		var x1 = tst.device_name
		tst.device_name = "my device name"
		compare(tst.device_name, "my device name")

		var x1 = tst.download_mode
		tst.download_mode = 19
		compare(tst.download_mode, 19)

		var x1 = tst.product
		tst.product = "my product"
		compare(tst.product, "my product")

		var x1 = tst.vendor
		tst.vendor = "my vendor"
		compare(tst.vendor, "my vendor")
	}
}

