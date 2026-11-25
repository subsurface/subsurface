// SPDX-License-Identifier: GPL-2.0
// AI-generated (Claude)
#include "desktop-widgets/updatecheck.h"

#include <QCoreApplication>
#include <QRegularExpression>

// The strings are translated in the context of UpdateManager, which is where
// they were before they moved here, so that existing translations stay valid.

QString updateCheckBuildParameters(build_kind kind, int prNumber)
{
	switch (kind) {
	case build_kind::pr:
		if (prNumber > 0)
			return QString("&build=pr&pr=%1").arg(prNumber);
		return QStringLiteral("&build=pr");
	case build_kind::ci:
		return QStringLiteral("&build=ci");
	case build_kind::local:
	default:
		return QStringLiteral("&build=local");
	}
}

// The version of the newest release if the server told us, empty otherwise.
static QString newestVersion(const QString &reply)
{
	static const QRegularExpression re("(?:Newest release version|Latest version) is \"?(\\d+(?:\\.\\d+)+)");
	QRegularExpressionMatch match = re.match(reply);
	return match.hasMatch() ? match.captured(1) : QString();
}

// The statement that a pull request test build is not the latest version.
static QString pullRequestStatement(int prNumber)
{
	if (prNumber > 0)
		return QCoreApplication::translate("UpdateManager", "You are running a test build of PR #%1, not the latest version.")
			.arg(prNumber);
	return QCoreApplication::translate("UpdateManager", "You are running a pull request test build, not the latest version.");
}

// The reminder of a pull request test build that it is not the latest version,
// and where to get it.
static QString pullRequestReminder(int prNumber, const QString &version)
{
	QString text = "<b>" + pullRequestStatement(prNumber) + "</b><br/><br/>";

	const QString releasesOpen = "<a href=\"https://github.com/subsurface/subsurface/releases\">";
	if (version.isEmpty())
		text += QCoreApplication::translate("UpdateManager", "Please check %1 our download page %2 for the latest release, or the %3 CI builds %4 for the latest development version.")
			.arg("<a href=\"https://subsurface-divelog.org/download\">", "</a>", releasesOpen, "</a>");
	else
		text += QCoreApplication::translate("UpdateManager", "The latest release is version %1. Please check %2 our download page %3 for it, or the %4 CI builds %5 for the latest development version.")
			.arg(version, "<a href=\"https://subsurface-divelog.org/download\">", "</a>", releasesOpen, "</a>");
	return text;
}

static QString localBuildMessage(const QString &version)
{
	QString text = QCoreApplication::translate("UpdateManager", "You are running a local build of Subsurface, which cannot be compared with the latest version.");
	text += "<br/><br/>";
	if (!version.isEmpty())
		text += QCoreApplication::translate("UpdateManager", "The latest release is version %1.").arg(version) + " ";
	text += QCoreApplication::translate("UpdateManager", "Please check %1 our download page %2 for the latest release.")
		.arg("<a href=\"https://subsurface-divelog.org/download\">", "</a>");
	return text;
}

static QString genericMessage(QString reply)
{
	// the webservice backend doesn't localize - but it's easy enough to just replace the
	// strings that it is likely to send back
	if (reply.contains("Newest release version is "))
		reply.replace("Newest release version is ", QCoreApplication::translate("UpdateManager", "Newest release version is "));
	return QCoreApplication::translate("UpdateManager", "The server returned the following information:").append("<br/><br/>").append(reply);
}

UpdateCheckOutcome evaluateUpdateReply(build_kind kind, int prNumber, bool automatic, const QString &reply)
{
	UpdateCheckOutcome result;
	QString link;
	if (reply.contains('"'))
		link = reply.split("\"").at(1);

	if (reply == "OK") {
		result.showPopup = !automatic;
		result.message = QCoreApplication::translate("UpdateManager", "You are using the latest version of Subsurface.");
	} else if (reply.startsWith("[\"http")) {
		result.showPopup = true;
		result.message = QCoreApplication::translate("UpdateManager", "A new version of Subsurface is available.<br/>Click on:<br/><a href=\"%1\">%1</a><br/> to download it.")
				.arg(link);
	} else if (reply.startsWith("Latest version")) {
		result.showPopup = true;
		result.message = QString("<b>") + QCoreApplication::translate("UpdateManager", "A new version of Subsurface is available.") + QString("</b><br/><br/>") +
				QCoreApplication::translate("UpdateManager", "Latest version is %1, please check %2 our download page %3 for information in how to update.")
				.arg(link).arg("<a href=\"http://subsurface-divelog.org/download\">").arg("</a>");
	} else if (reply.contains("There is a newer release")) {
		// a real newer release exists, this is always worth a popup
		result.showPopup = true;
		QString version = newestVersion(reply);
		QString text = "<b>" + QCoreApplication::translate("UpdateManager", "A newer release of Subsurface is available.") + "</b><br/><br/>";
		if (!version.isEmpty())
			text += QCoreApplication::translate("UpdateManager", "The latest release is version %1.").arg(version) + " ";
		text += QCoreApplication::translate("UpdateManager", "Please check %1 our download page %2 for information in how to update.")
				.arg("<a href=\"https://subsurface-divelog.org/download\">", "</a>");
		if (kind == build_kind::pr)
			text = "<b>" + pullRequestStatement(prNumber) + "</b><br/><br/>" + text;
		result.message = text;
	} else if (reply.contains("latest development") || reply.contains("newer") ||
		   reply.contains("beta", Qt::CaseInsensitive) || reply.contains("local build")) {
		// the server tells us that this is not a plain release, how that is worded
		// depends on the kind of build
		switch (kind) {
		case build_kind::pr:
			// somebody testing a pull request is reminded every time that this is not the latest version
			result.showPopup = true;
			result.message = pullRequestReminder(prNumber, newestVersion(reply));
			break;
		case build_kind::local:
			result.showPopup = !automatic;
			result.message = localBuildMessage(newestVersion(reply));
			break;
		case build_kind::ci:
		default:
			result.showPopup = !automatic;
			result.warning = true;
			result.message = genericMessage(reply);
			break;
		}
	} else {
		// not recognized, so show what the server said
		result.showPopup = true;
		result.warning = true;
		result.message = genericMessage(reply);
	}
	return result;
}
