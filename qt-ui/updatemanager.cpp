#include "updatemanager.h"
#include "usersurvey.h"
#include <QtNetwork>
#include <QMessageBox>
#include "subsurfacewebservices.h"
#include "ssrf-version.h"
#include "mainwindow.h"

UpdateManager::UpdateManager(QObject *parent) : QObject(parent)
{
	// is this the first time this version was run?
	QSettings settings;
	settings.beginGroup("UpdateManager");
	if (settings.contains("DontCheckForUpdates") && settings.value("DontCheckForUpdates") == "TRUE")
		return;
	if (settings.contains("LastVersionUsed")) {
		// we have checked at least once before
		if (settings.value("LastVersionUsed").toString() != GIT_VERSION_STRING) {
			// we have just updated - wait two weeks before you check again
			settings.setValue("LastVersionUsed", QString(GIT_VERSION_STRING));
			settings.setValue("NextCheck", QDateTime::currentDateTime().addDays(14).toString(Qt::ISODate));
			return;
		}
		// is it time to check again?
		QString nextCheckString = settings.value("NextCheck").toString();
		QDateTime nextCheck = QDateTime::fromString(nextCheckString, Qt::ISODate);
		if (nextCheck > QDateTime::currentDateTime())
			return;
	}
	settings.setValue("LastVersionUsed", QString(GIT_VERSION_STRING));
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
	qDebug() << "checking for update";
	isAutomaticCheck = automatic;
	QString version = CANONICAL_VERSION_STRING;
	QString url = QString("http://subsurface-divelog.org/updatecheck.html?os=%1&version=%2").arg(os, version);
	QNetworkRequest request;
	request.setUrl(url);
	request.setRawHeader("Accept", "text/xml");
	QString userAgent = UserSurvey::getUserAgent();
	request.setRawHeader("User-Agent", userAgent.toUtf8());
	connect(SubsurfaceWebServices::manager()->get(request), SIGNAL(finished()), this, SLOT(requestReceived()));
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
		QString response(reply->readAll());
		QString responseBody;
		if (response.contains('"'))
			responseBody = response.split("\"").at(1);
		else
			responseBody = response;

		msgbox.setIcon(QMessageBox::Information);

		if (responseBody == "OK") {
			msgText = tr("You are using the latest version of Subsurface.");
		} else if (responseBody.startsWith("http")) {
			haveNewVersion = true;
			msgText = tr("A new version of Subsurface is available.<br/>Click on:<br/><a href=\"%1\">%1</a><br/> to download it.")
					.arg(responseBody);
		} else if (responseBody.startsWith("Latest version")) {
			// the webservice backend doesn't localize - but it's easy enough to just replace the
			// strings that it is likely to send back
			haveNewVersion = true;
			responseBody.replace("Latest version is ", "");
			responseBody.replace(". please check with your OS vendor for updates.", "");
			msgText = QString("<b>") + tr("A new version of Subsurface is available.") + QString("</b><br/><br/>") +
					tr("Latest version is %1, please check with your OS vendor for updates.")
					.arg(responseBody);
		} else {
			// the webservice backend doesn't localize - but it's easy enough to just replace the
			// strings that it is likely to send back
			haveNewVersion = true;
			if (responseBody.contains("Newest release version is "))
				responseBody.replace("Newest release version is ", tr("Newest release version is "));
			msgText = tr("The server returned the following information:").append("<br/><br/>").append(responseBody);
			msgbox.setIcon(QMessageBox::Warning);
		}
	}
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
}
