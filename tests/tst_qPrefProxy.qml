// SPDX-License-Identifier: GPL-2.0
import QtQuick 2.6
import QtTest 1.2
import org.subsurfacedivelog.mobile 1.0

TestCase {
	name: "qPrefProxy"

	SsrfProxyPrefs {
		id: tst
	}

	SsrfPrefs {
		id: prefs
	}

	function test_variables() {
		var x1 = tst.proxy_auth
		tst.proxy_auth = true
		compare(tst.proxy_auth, true)

		var x2 = tst.proxy_host
		tst.proxy_host = "my host"
		compare(tst.proxy_host, "my host")

		var x3 = tst.proxy_pass
		tst.proxy_pass = "my pass"
		compare(tst.proxy_pass, "my pass")

		var x3 = tst.proxy_port
		tst.proxy_port = 544
		compare(tst.proxy_port, 544)

		var x5 = tst.proxy_type
		tst.proxy_type = 3
		compare(tst.proxy_type, 3)

		var x3 = tst.proxy_user
		tst.proxy_user = "my user"
		compare(tst.proxy_user, "my user")
	}
}
