// SPDX-License-Identifier: GPL-2.0
import QtQuick 2.6
import QtTest 1.2

TestCase {
	name: "qPrefProxy"

	function test_variables() {
		var x1 = PrefProxy.proxy_auth
		PrefProxy.proxy_auth = true
		compare(PrefProxy.proxy_auth, true)

		var x2 = PrefProxy.proxy_host
		PrefProxy.proxy_host = "my host"
		compare(PrefProxy.proxy_host, "my host")

		var x3 = PrefProxy.proxy_pass
		PrefProxy.proxy_pass = "my pass"
		compare(PrefProxy.proxy_pass, "my pass")

		var x4 = PrefProxy.proxy_port
		PrefProxy.proxy_port = 544
		compare(PrefProxy.proxy_port, 544)

		var x5 = PrefProxy.proxy_type
		PrefProxy.proxy_type = 3
		compare(PrefProxy.proxy_type, 3)

		var x6 = PrefProxy.proxy_user
		PrefProxy.proxy_user = "my user"
		compare(PrefProxy.proxy_user, "my user")
	}

	Item {
		id: spyCatcher

		property bool spy1 : false
		property bool spy2 : false
		property bool spy3 : false
		property bool spy4 : false
		property bool spy5 : false
		property bool spy6 : false

		Connections {
			target: PrefProxy
			onProxy_authChanged: {spyCatcher.spy1 = true }
			onProxy_hostChanged: {spyCatcher.spy2 = true }
			onProxy_passChanged: {spyCatcher.spy3 = true }
			onProxy_portChanged: {spyCatcher.spy4 = true }
			onProxy_typeChanged: {spyCatcher.spy5 = true }
			onProxy_userChanged: {spyCatcher.spy6 = true }
		}
	}

	function test_signals() {
		PrefProxy.proxy_auth = ! PrefProxy.proxy_auth
		PrefProxy.proxy_host = "qml"
		PrefProxy.proxy_pass = "qml"
		PrefProxy.proxy_port = -544
		PrefProxy.proxy_type = -3
		PrefProxy.proxy_user = "qml"

		compare(spyCatcher.spy1, true)
		compare(spyCatcher.spy2, true)
		compare(spyCatcher.spy3, true)
		compare(spyCatcher.spy4, true)
		compare(spyCatcher.spy5, true)
		compare(spyCatcher.spy6, true)
	}
}
