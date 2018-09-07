// SPDX-License-Identifier: GPL-2.0
import QtQuick 2.6
import QtTest 1.2

TestCase {
	name: "qPrefFacebook"

	function test_variables() {
		var x1 = PrefFacebook.access_token
		PrefFacebook.access_token = "my token"
		compare(PrefFacebook.access_token, "my token")

		var x2 = PrefFacebook.album_id
		PrefFacebook.album_id = "my album"
		compare(PrefFacebook.album_id, "my album")

		var x3 = PrefFacebook.user_id
		PrefFacebook.user_id = "my user"
		compare(PrefFacebook.user_id, "my user")
	}
}
