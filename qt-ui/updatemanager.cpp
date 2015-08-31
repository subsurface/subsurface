#include "updatemanager.h"
#include "helpers.h"
#include <QtNetwork>
#include <QMessageBox>
#include <QUuid>
#include "subsurfacewebservices.h"
#include "version.h"
#include "mainwindow.h"

UpdateManager::UpdateManager(QObject *parent) :
	QObject(parent),
	isAutomaticCheck(false)
{
	// is this the first time this version was run?
	QSettings settings;
	settings.beginGroup("UpdateManager");
	if (settings.contains("DontCheckForUpdates") && settings.value("DontCheckForUpdates") == "TRUE")
		return;
	if (settings.contains("LastVersionUsed")) {
		// we have checked at least once before
		if (settings.value("LastVersionUsed").toString() != subsurface_git_version()) {
			// we have just updated - wait two weeks before you check again
			settings.setValue("LastVersionUsed", QString(subsurface_git_version()));
			settings.setValue("NextCheck", QDateTime::currentDateTime().addDays(14).toString(Qt::ISODate));
		} else {
			// is it time to check again?
			QString nextCheckString = settings.value("NextCheck").toString();
			QDateTime nextCheck = QDateTime::fromString(nextCheckString, Qt::ISODate);
			if (nextCheck > QDateTime::currentDateTime())
				return;
		}
	}
	settings.setValue("LastVersionUsed", QString(subsurface_git_version()));
	settings.setValue("NextCheck", QDateTime::currentDateTime().addDays(14).toString(Qt::ISODate));
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
	QString url = QString("http://subsurface-divelog.org/updatecheck.html?os=%1&version=%2&uuid=%3").arg(os, version, uuidString);
	QNetworkRequest request;
	request.setUrl(url);
	request.setRawHeader("Accept", "text/xml");
	QString userAgent = getUserAgent();
	request.setRawHeader("User-Agent", userAgent.toUtf8());
	connect(SubsurfaceWebServices::manager()->get(request), SIGNAL(finished()), this, SLOT(requestReceived()), Qt::UniqueConnection);
}

QString UpdateManager::getUUID()
{
	QString uuidString;
	QSettings settings;
	settings.beginGroup("UpdateManager");
	if (settings.contains("UUID")) {
		uuidString = settings.value("UUID").toString();
	} else {
		QUuid uuid = QUuid::createUuid();
		uuidString = uuid.toString();
		settings.setValue("UUID", uuidString);
	}
	uuidString.replace("{", "").replace("}", "");
	return uuidString;
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
#ifndef SUBSURFACE_MOBILE
	if (haveNewVersion || !isAutomaticCheck) {
		msgbox.setWindowTitle(msgTitle);
		msgbox.setWindowIcon(QIcon(":/subsurface-icon"));
		msgbox.setText(msgText);
		msgbox.setTextFormat(Qt::RichText);
		msgbox.exec();
	}
	if (isAutomaticCheck) {
		QSettings settings;
		settings.beginGroup("UpdateManager");
		if (!settings.contains("DontCheckForUpdates")) {
			// we allow an opt out of future checks
			QMessageBox response(MainWindow::instance());
			QString message = tr("Subsurface is checking every two weeks if a new version is available. If you don't want Subsurface to continue checking, please click Decline.");
			response.addButton(tr("Decline"), QMessageBox::RejectRole);
			response.addButton(tr("Accept"), QMessageBox::AcceptRole);
			response.setText(message);
			response.setWindowTitle(tr("Automatic check for updates"));
			response.setIcon(QMessageBox::Question);
			response.setWindowModality(Qt::WindowModal);
			int ret = response.exec();
			if (ret == QMessageBox::Accepted)
				settings.setValue("DontCheckForUpdates", "FALSE");
			else
				settings.setValue("DontCheckForUpdates", "TRUE");
		}
	}
#endif
}
