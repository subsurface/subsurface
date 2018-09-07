// SPDX-License-Identifier: GPL-2.0
import QtQuick 2.6
import QtTest 1.2

TestCase {
	name: "qPrefDiveComputer"

	function test_variables() {
		var x1 = PrefDiveComputer.device
		PrefDiveComputer.device = "my device"
		compare(PrefDiveComputer.device, "my device")

		var x2 = PrefDiveComputer.device_name
		PrefDiveComputer.device_name = "my device name"
		compare(PrefDiveComputer.device_name, "my device name")

		var x3 = PrefDiveComputer.download_mode
		PrefDiveComputer.download_mode = 19
		compare(PrefDiveComputer.download_mode, 19)

		var x4 = PrefDiveComputer.product
		PrefDiveComputer.product = "my product"
		compare(PrefDiveComputer.product, "my product")

		var x5 = PrefDiveComputer.vendor
		PrefDiveComputer.vendor = "my vendor"
		compare(PrefDiveComputer.vendor, "my vendor")
	}
}

