// SPDX-License-Identifier: GPL-2.0
#include "desktop-widgets/updatemanager.h"
#include "desktop-widgets/updatecheck.h"
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
	QString url = QString("http://updatecheck.subsurface-divelog.org/updatecheck.html?os=%1&version=%2").arg(os, version) +
		      updateCheckBuildParameters(subsurface_build_kind(), subsurface_pr_number());
	QNetworkRequest request;
	request.setUrl(url);
	request.setRawHeader("Accept", "text/xml");
	QString userAgent = getUserAgent();
	request.setRawHeader("User-Agent", userAgent.toUtf8());
	connect(manager()->get(request), SIGNAL(finished()), this, SLOT(requestReceived()), Qt::UniqueConnection);
}

void UpdateManager::requestReceived()
{
	bool showPopup = !isAutomaticCheck;
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
		UpdateCheckOutcome outcome = evaluateUpdateReply(subsurface_build_kind(), subsurface_pr_number(),
								 isAutomaticCheck, responseBody);
		showPopup = outcome.showPopup;
		msgText = outcome.message;
		msgbox.setIcon(outcome.warning ? QMessageBox::Warning : QMessageBox::Information);
	}
	if (showPopup) {
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
