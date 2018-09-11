// SPDX-License-Identifier: GPL-2.0
import QtQuick 2.6
import QtTest 1.2
import org.subsurfacedivelog.mobile 1.0

TestCase {
	name: "qPrefCloudStorage"

	SsrfCloudStoragePrefs {
		id: tst
	}

	SsrfPrefs {
		id: prefs
	}

	function test_variables() {
		var x1 = tst.cloud_base_url
		tst.cloud_base_url = "my url"
		compare(tst.cloud_base_url, "my url")

		var x2 = tst.cloud_git_url
		compare(tst.cloud_git_url, "my url/git")

		var x3 = tst.cloud_storage_email
		tst.cloud_storage_email = "my email"
		compare(tst.cloud_storage_email, "my email")

		var x4 = tst.cloud_storage_email_encoded
		tst.cloud_storage_email_encoded= "my email enc"
		compare(tst.cloud_storage_email_encoded, "my email enc")

		var x6 = tst.cloud_storage_password
		tst.cloud_storage_password = "my url"
		compare(tst.cloud_storage_password, "my url")

		var x7 = tst.cloud_storage_pin
		tst.cloud_storage_pin= "my pin"
		compare(tst.cloud_storage_pin, "my pin")

		var x8 = tst.cloud_timeout
		tst.cloud_timeout = 137
		compare(tst.cloud_timeout, 137)

		var x9 = tst.cloud_verification_status
		tst.cloud_verification_status = SsrfPrefs.CS_VERIFIED
		compare(tst.cloud_verification_status, SsrfPrefs.CS_VERIFIED)

		var x11 = tst.save_password_local
		tst.save_password_local = true
		compare(tst.save_password_local, true)

		var x12 = tst.save_userid_local
		tst.save_userid_local = true
		compare(tst.save_userid_local, true)

		var x13 = tst.userid
		tst.userid = "my user"
		compare(tst.userid, "my user")
	}
}
