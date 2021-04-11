// SPDX-License-Identifier: GPL-2.0
import QtQuick 2.6
import QtTest 1.2
import org.subsurfacedivelog.mobile 1.0

TestCase {
	name: "qPrefCloudStorage"

	function test_variables() {
		var x1 = PrefCloudStorage.cloud_base_url
		PrefCloudStorage.cloud_base_url = "my url"
		compare(PrefCloudStorage.cloud_base_url, "my url")

		var x3 = PrefCloudStorage.cloud_storage_email
		PrefCloudStorage.cloud_storage_email = "my email"
		compare(PrefCloudStorage.cloud_storage_email, "my email")

		var x4 = PrefCloudStorage.cloud_storage_email_encoded
		PrefCloudStorage.cloud_storage_email_encoded= "my email enc"
		compare(PrefCloudStorage.cloud_storage_email_encoded, "my email enc")

		var x5 = PrefCloudStorage.cloud_storage_password
		PrefCloudStorage.cloud_storage_password = "my url"
		compare(PrefCloudStorage.cloud_storage_password, "my url")

		var x6 = PrefCloudStorage.cloud_storage_pin
		PrefCloudStorage.cloud_storage_pin= "my pin"
		compare(PrefCloudStorage.cloud_storage_pin, "my pin")

		var x7 = PrefCloudStorage.cloud_timeout
		PrefCloudStorage.cloud_timeout = 137
		compare(PrefCloudStorage.cloud_timeout, 137)

		var x8 = PrefCloudStorage.cloud_verification_status
		PrefCloudStorage.cloud_verification_status = 1
		PrefCloudStorage.cloud_verification_status = CloudStatus.CS_VERIFIED
		compare(PrefCloudStorage.cloud_verification_status, CloudStatus.CS_VERIFIED)

		var x9 = PrefCloudStorage.save_password_local
		PrefCloudStorage.save_password_local = true
		compare(PrefCloudStorage.save_password_local, true)
	}

	Item {
		id: spyCatcher

		property bool spy1 : false
		property bool spy3 : false
		property bool spy4 : false
		property bool spy5 : false
		property bool spy6 : false
		property bool spy7 : false
		property bool spy8 : false
		property bool spy9 : false

		Connections {
			target: PrefCloudStorage
			onCloud_base_urlChanged: {spyCatcher.spy1 = true }
			onCloud_storage_emailChanged: {spyCatcher.spy3 = true }
			onCloud_storage_email_encodedChanged: {spyCatcher.spy4 = true }
			onCloud_storage_passwordChanged: {spyCatcher.spy5 = true }
			onCloud_storage_pinChanged: {spyCatcher.spy6 = true }
			onCloud_timeoutChanged: {spyCatcher.spy7 = true }
			onCloud_verification_statusChanged: {spyCatcher.spy8 = true }
			onSave_password_localChanged: {spyCatcher.spy9 = true }
		}
	}

	function test_signals() {
		PrefCloudStorage.cloud_base_url = "qml"
		PrefCloudStorage.cloud_storage_email = "qml"
		PrefCloudStorage.cloud_storage_email_encoded = "qml"
		PrefCloudStorage.cloud_storage_password = "qml"
		PrefCloudStorage.cloud_storage_pin = "qml"
		PrefCloudStorage.cloud_timeout = 18
		PrefCloudStorage.cloud_verification_status = 2
		PrefCloudStorage.save_password_local = ! PrefCloudStorage.save_password_local

		compare(spyCatcher.spy1, true)
		compare(spyCatcher.spy3, true)
		compare(spyCatcher.spy4, true)
		compare(spyCatcher.spy5, true)
		compare(spyCatcher.spy6, true)
		compare(spyCatcher.spy7, true)
		compare(spyCatcher.spy8, true)
		compare(spyCatcher.spy9, true)
	}
}
