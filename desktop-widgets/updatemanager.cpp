// SPDX-License-Identifier: GPL-2.0
#include "desktop-widgets/updatemanager.h"
#include "core/qthelper.h"
#include <QtNetwork>
#include <QMessageBox>
#include <QUuid>
#include "desktop-widgets/subsurfacewebservices.h"
#include "core/version.h"
#include "desktop-widgets/mainwindow.h"
#include "core/cloudstorage.h"
#include "core/settings/qPrefUpdateManager.h"

UpdateManager::UpdateManager(QObject *parent) :
	QObject(parent),
	isAutomaticCheck(false)
{
	if (qPrefUpdateManager::dont_check_for_updates())
		return;

	if (qPrefUpdateManager::last_version_used() == subsurface_git_version() &&
	    qPrefUpdateManager::next_check() > QDate::currentDate())
		return;

	qPrefUpdateManager::set_last_version_used(subsurface_git_version());

	checkForUpdates(true);
}

void UpdateManager::checkForUpdates(bool automatic)
{
	QString os;

#if defined(Q_OS_WIN)
	os = "win";
#elif defined(Q_OS_MAC)
	os = "osx";
#elif defined(Q_OS_LINUX)
	os = "linux";
#else
	os = "unknown";
#endif
	isAutomaticCheck = automatic;
	QString version = subsurface_canonical_version();
	QString uuidString = getUUID();
	QString url = QString("http://updatecheck.subsurface-divelog.org/updatecheck.html?os=%1&version=%2&uuid=%3").arg(os, version, uuidString);
	QNetworkRequest request;
	request.setUrl(url);
	request.setRawHeader("Accept", "text/xml");
	QString userAgent = getUserAgent();
	request.setRawHeader("User-Agent", userAgent.toUtf8());
	connect(manager()->get(request), SIGNAL(finished()), this, SLOT(requestReceived()), Qt::UniqueConnection);
}

void UpdateManager::requestReceived()
{
	bool haveNewVersion = false;
	QMessageBox msgbox;
	QString msgTitle = tr("Check for updates.");
	QString msgText = "<h3>" + tr("Subsurface was unable to check for updates.") + "</h3>";

	QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
	if (reply->error() != QNetworkReply::NoError) {
		//Network Error
		msgText = msgText + "<br/><b>" + tr("The following error occurred:") + "</b><br/>" + reply->errorString()
				+ "<br/><br/><b>" + tr("Please check your internet connection.") + "</b>";
	} else {
		//No network error
		QString responseBody(reply->readAll());
		QString responseLink;
		if (responseBody.contains('"'))
			responseLink = responseBody.split("\"").at(1);

		msgbox.setIcon(QMessageBox::Information);
		if (responseBody == "OK") {
			msgText = tr("You are using the latest version of Subsurface.");
		} else if (responseBody.startsWith("[\"http")) {
			haveNewVersion = true;
			msgText = tr("A new version of Subsurface is available.<br/>Click on:<br/><a href=\"%1\">%1</a><br/> to download it.")
					.arg(responseLink);
		} else if (responseBody.startsWith("Latest version")) {
			// the webservice backend doesn't localize - but it's easy enough to just replace the
			// strings that it is likely to send back
			haveNewVersion = true;
			msgText = QString("<b>") + tr("A new version of Subsurface is available.") + QString("</b><br/><br/>") +
					tr("Latest version is %1, please check %2 our download page %3 for information in how to update.")
					.arg(responseLink).arg("<a href=\"http://subsurface-divelog.org/download\">").arg("</a>");
		} else {
			// the webservice backend doesn't localize - but it's easy enough to just replace the
			// strings that it is likely to send back
			if (!responseBody.contains("latest development") &&
			    !responseBody.contains("newer") &&
			    !responseBody.contains("beta", Qt::CaseInsensitive))
				haveNewVersion = true;
			if (responseBody.contains("Newest release version is "))
				responseBody.replace("Newest release version is ", tr("Newest release version is "));
			msgText = tr("The server returned the following information:").append("<br/><br/>").append(responseBody);
			msgbox.setIcon(QMessageBox::Warning);
		}
	}
	if (haveNewVersion || !isAutomaticCheck) {
		msgbox.setWindowTitle(msgTitle);
		msgbox.setWindowIcon(QIcon(":subsurface-icon"));
		msgbox.setText(msgText);
		msgbox.setTextFormat(Qt::RichText);
		msgbox.exec();
	}
	if (isAutomaticCheck) {
		auto update_settings = qPrefUpdateManager::instance();
		if (!update_settings->dont_check_for_updates() && update_settings->next_check() == QDate::fromJulianDay(0)) {
			// we allow an opt out of future checks
			QMessageBox response(MainWindow::instance());
			QString message = tr("Subsurface is checking every two weeks if a new version is available. "
								 "\nIf you don't want Subsurface to continue checking, please click Decline.");
			response.addButton(tr("Decline"), QMessageBox::RejectRole);
			response.addButton(tr("Accept"), QMessageBox::AcceptRole);
			response.setText(message);
			response.setWindowTitle(tr("Automatic check for updates"));
			response.setIcon(QMessageBox::Question);
			response.setWindowModality(Qt::WindowModal);
			update_settings->set_dont_check_for_updates(response.exec() != QMessageBox::Accepted);
		}
	}
	qPrefUpdateManager::set_next_check(QDate::currentDate().addDays(14));
}
