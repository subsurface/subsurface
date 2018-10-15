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

		var x4 = PrefDiveComputer.product
		PrefDiveComputer.product = "my product"
		compare(PrefDiveComputer.product, "my product")

		var x5 = PrefDiveComputer.vendor
		PrefDiveComputer.vendor = "my vendor"
		compare(PrefDiveComputer.vendor, "my vendor")
	}

	Item {
		id: spyCatcher

		property bool spy1 : false
		property bool spy2 : false
		property bool spy4 : false
		property bool spy5 : false

		Connections {
			target: PrefDiveComputer
			onDeviceChanged: {spyCatcher.spy1 = true }
			onDevice_nameChanged: {spyCatcher.spy2 = true }
			onProductChanged: {spyCatcher.spy4 = true }
			onVendorChanged: {spyCatcher.spy5 = true }
		}
	}

	function test_signals() {
		PrefDiveComputer.device = "qml"
		PrefDiveComputer.device_name = "qml"
		PrefDiveComputer.product = "qml"
		PrefDiveComputer.vendor = "qml"

		compare(spyCatcher.spy1, true)
		compare(spyCatcher.spy2, true)
		compare(spyCatcher.spy4, true)
		compare(spyCatcher.spy5, true)
	}
}

