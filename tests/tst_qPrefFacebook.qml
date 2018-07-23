// SPDX-License-Identifier: GPL-2.0
import QtQuick 2.6
import QtTest 1.2
import org.subsurfacedivelog.mobile 1.0

TestCase {
	name: "qPrefFacebook"

	SsrfFacebookPrefs {
		id: facebook
	}

	function test_variables() {
		var x1 = facebook.access_token
		facebook.access_token = "my token"
		compare(facebook.access_token, "my token")
		var x2 = facebook.album_id
		facebook.album_id = "my album"
		compare(facebook.album_id, "my album")
		var x1 = facebook.user_id
		facebook.user_id = "my user"
		compare(facebook.user_id, "my user")
	}
}
