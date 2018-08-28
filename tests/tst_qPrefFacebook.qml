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

	Item {
		id: spyCatcher

		property bool spy1 : false
		property bool spy2 : false
		property bool spy3 : false

		Connections {
			target: PrefFacebook
			onAccess_tokenChanged: {spyCatcher.spy1 = true }
			onAlbum_idChanged: {spyCatcher.spy2 = true }
			onUser_idChanged: {spyCatcher.spy3 = true }
		}
	}

	function test_signals() {
		PrefFacebook.access_token = "qml"
		PrefFacebook.album_id = "qml"
		PrefFacebook.user_id = "qml"

		compare(spyCatcher.spy1, true)
		compare(spyCatcher.spy2, true)
		compare(spyCatcher.spy3, true)
	}
}
