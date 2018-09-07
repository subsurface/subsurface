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
}
