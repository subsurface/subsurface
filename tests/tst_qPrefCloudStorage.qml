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

		var x2 = PrefCloudStorage.cloud_git_url
		compare(PrefCloudStorage.cloud_git_url, "my url/git")

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

		var x10 = PrefCloudStorage.save_userid_local
		PrefCloudStorage.save_userid_local = true
		compare(PrefCloudStorage.save_userid_local, true)

		var x11 = PrefCloudStorage.userid
		PrefCloudStorage.userid = "my user"
		compare(PrefCloudStorage.userid, "my user")
	}
}
