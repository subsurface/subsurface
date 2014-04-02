#include "updatemanager.h"
#include <QtNetwork>
#include <QMessageBox>
#include "subsurfacewebservices.h"
#include "ssrf-version.h"

UpdateManager::UpdateManager(QObject *parent) :
	QObject(parent)
{
	manager = SubsurfaceWebServices::manager();
	connect (manager, SIGNAL(finished(QNetworkReply*)), SLOT(requestReceived(QNetworkReply*)));
}

void UpdateManager::checkForUpdates()
{
	QString os;

#if defined(Q_OS_WIN)
	os = "win";
#elif defined(Q_OS_MAC)
	os = "osx";
#else
	os = "unknown";
#endif

	QString version = VERSION_STRING;
	QString url = QString("http://subsurface.hohndel.org/updatecheck.html?os=%1&ver=%2").arg(os, version);
	manager->get(QNetworkRequest(QUrl(url)));
}

void UpdateManager::requestReceived(QNetworkReply *reply)
{
	QMessageBox msgbox;
	QString msgTitle = tr("Check for updates.");
	QString msgText = tr("<h3>Subsurface was unable to check for updates.</h3>");


	if (reply->error() != QNetworkReply::NoError) {
		//Network Error
		msgText = msgText + tr("<br/><b>The following error occurred:</b><br/>") + reply->errorString()
				+ tr("<br/><br/><b>Please check your internet connection.</b>");
	} else {
		//No network error
		QString response(reply->readAll());
		QString responseBody = response.split("\"").at(1);

		msgbox.setIcon(QMessageBox::Information);

		if (responseBody == "OK") {
			msgText = tr("You are using the latest version of subsurface.");
		} else if (responseBody.startsWith("http")) {
			msgText = tr("A new version of subsurface is available.<br/>Click on:<br/><a href=\"%1\">%1</a><br/> to download it.")
					.arg(responseBody);
		} else if (responseBody.startsWith("Latest version")) {
			msgText = tr("<b>A new version of subsurface is available.</b><br/><br/>%1")
					.arg(responseBody);
		} else {
			msgText = tr("There was an error while trying to check for updates.<br/><br/>%1").arg(responseBody);
			msgbox.setIcon(QMessageBox::Warning);
		}
	}

	msgbox.setWindowTitle(msgTitle);
	msgbox.setText(msgText);
	msgbox.setTextFormat(Qt::RichText);
	msgbox.exec();
}
