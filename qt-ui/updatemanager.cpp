#include "updatemanager.h"
#include "usersurvey.h"
#include <QtNetwork>
#include <QMessageBox>
#include "subsurfacewebservices.h"
#include "ssrf-version.h"

UpdateManager::UpdateManager(QObject *parent) : QObject(parent)
{
}

void UpdateManager::checkForUpdates()
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

	QString version = VERSION_STRING;
	QString url = QString("http://subsurface-divelog.org/updatecheck.html?os=%1&ver=%2").arg(os, version);
	QNetworkRequest request;
	request.setUrl(url);
	request.setRawHeader("Accept", "text/xml");
	QString userAgent = UserSurvey::getUserAgent();
	request.setRawHeader("User-Agent", userAgent.toUtf8());
	connect(SubsurfaceWebServices::manager()->get(request), SIGNAL(finished()), this, SLOT(requestReceived()));
}

void UpdateManager::requestReceived()
{
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
			msgText = tr("You are using the latest version of subsurface.");
		} else if (responseBody.startsWith("http")) {
			msgText = tr("A new version of subsurface is available.<br/>Click on:<br/><a href=\"%1\">%1</a><br/> to download it.")
					.arg(responseBody);
		} else if (responseBody.startsWith("Latest version")) {
			// the webservice backend doesn't localize - but it's easy enough to just replace the
			// strings that it is likely to send back
			responseBody.replace("Latest version is ", "");
			responseBody.replace(". please check with your OS vendor for updates.", "");
			msgText = QString("<b>") + tr("A new version of subsurface is available.") + QString("</b><br/><br/>") +
					tr("Latest version is %1, please check with your OS vendor for updates.")
					.arg(responseBody);
		} else {
			// the webservice backend doesn't localize - but it's easy enough to just replace the
			// strings that it is likely to send back
			if (responseBody.contains("Newest release version is "))
				responseBody.replace("Newest release version is ", tr("Newest release version is "));
			msgText = tr("There was an error while trying to check for updates.<br/><br/>%1").arg(responseBody);
			msgbox.setIcon(QMessageBox::Warning);
		}
	}

	msgbox.setWindowTitle(msgTitle);
	msgbox.setWindowIcon(QIcon(":/subsurface-icon"));
	msgbox.setText(msgText);
	msgbox.setTextFormat(Qt::RichText);
	msgbox.exec();
}
