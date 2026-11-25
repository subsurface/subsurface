// SPDX-License-Identifier: GPL-2.0
// AI-generated (Claude)
#ifndef UPDATECHECK_H
#define UPDATECHECK_H

#include "core/version.h"

#include <QString>

// What to do with the reply of the update server. This is kept apart from the
// network and dialog code of UpdateManager so that it can be tested.
struct UpdateCheckOutcome {
	bool showPopup = false;	// show the message in a dialog
	bool warning = false;	// use the warning instead of the information icon
	QString message;	// rich text
};

// The query parameters that tell the server which kind of build is asking, to
// be appended to the update check URL: "&build=local", "&build=ci", "&build=pr"
// and, for a pull request build with a known number, "&pr=<number>".
QString updateCheckBuildParameters(build_kind kind, int prNumber);

// Decide what to show for a reply of the update server. The wording is composed
// here from the kind of build; the reply is only used to find out whether there
// is a newer version, and which one.
UpdateCheckOutcome evaluateUpdateReply(build_kind kind, int prNumber, bool automatic, const QString &reply);

#endif // UPDATECHECK_H
