#!/bin/bash

# prep things so we can build for Linux
# we have a custom built Qt some gives us just what we need, including QtWebKit
#
# this is built from the latest version as of 2017-11-09 in the 5.9 branch and
# therefore calls itself Qt-5.9.3

set -x

# Travis only pulls shallow repos. But that messes with git describe.
# Sorry Travis, fetching the whole thing and the tags as well...
git fetch --unshallow
git pull --tags
git describe

. /opt/qt510/bin/qt510-env.sh
export QT_ROOT=/opt/qt510

# TestPreferences uses gui calls, so run a xvfb so it has something to talk to
export DISPLAY=:99.0
sh -e /etc/init.d/xvfb start

