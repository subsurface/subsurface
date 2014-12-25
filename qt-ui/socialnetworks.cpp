#include "socialnetworks.h"

#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QNetworkAccessManager>
#include <QUrlQuery>
#include <QEventLoop>
#include <QHttpMultiPart>
#include <QSettings>
#include <QFile>
#include <QBuffer>
#include <QDebug>
#include "mainwindow.h"
#include "profile/profilewidget2.h"
#include "pref.h"

#define GET_TXT(name, field)                                             \
	v = s.value(QString(name));                                      \
	if (v.isValid())                                                 \
		prefs.field = strdup(v.toString().toUtf8().constData()); \
	else                                                             \
		prefs.field = default_prefs.field

FacebookManager *FacebookManager::instance()
{
	static FacebookManager *self = new FacebookManager();
	return self;
}

FacebookManager::FacebookManager(QObject *parent) : QObject(parent)
{
	sync();
}

QUrl FacebookManager::connectUrl() {
	return QUrl("https://www.facebook.com/dialog/oauth?"
		"client_id=427722490709000"
		"&redirect_uri=http://www.facebook.com/connect/login_success.html"
		"&response_type=token"
		"&scope=publish_actions,user_photos"
	);
}

bool FacebookManager::loggedIn() {
	return prefs.facebook.access_token != NULL;
}

void FacebookManager::sync()
{
	qDebug() << "Sync Active";
	QSettings s;
	s.beginGroup("WebApps");
	s.beginGroup("Facebook");

	QVariant v;
	GET_TXT("ConnectToken", facebook.access_token);
	GET_TXT("UserId", facebook.user_id);
	GET_TXT("AlbumId", facebook.album_id);

	qDebug() << "Connection Token" << prefs.facebook.access_token;
	qDebug() << "User ID" << prefs.facebook.user_id;
	qDebug() << "Album ID" << prefs.facebook.album_id;
}

void FacebookManager::tryLogin(const QUrl& loginResponse)
{
	qDebug() << "Trying to Login.";
	QString result = loginResponse.toString();
	if (!result.contains("access_token")) {
		return;
	}

	qDebug() << "Login Sucessfull";
	int from = result.indexOf("access_token=") + strlen("access_token=");
	int to = result.indexOf("&expires_in");
	QString securityToken = result.mid(from, to-from);

	QSettings settings;
	settings.beginGroup("WebApps");
	settings.beginGroup("Facebook");
	settings.setValue("ConnectToken", securityToken);
	sync();
	requestUserId();
	sync();
	requestAlbumId();
	sync();
	emit justLoggedIn();
	qDebug() << "End try login";
}

void FacebookManager::logout()
{
	qDebug() << "Logging out";
	QSettings settings;
	settings.beginGroup("WebApps");
	settings.beginGroup("Facebook");
	settings.remove("ConnectToken");
	settings.remove("UserId");
	settings.remove("AlbumId");
	sync();
	emit justLoggedOut();
}

void FacebookManager::requestAlbumId()
{
	qDebug() << "Requesting Album ID";

	QUrl albumListUrl("https://graph.facebook.com/me/albums?access_token=" + QString(prefs.facebook.access_token));
	QNetworkAccessManager *manager = new QNetworkAccessManager();
	QNetworkReply *reply = manager->get(QNetworkRequest(albumListUrl));

	QEventLoop loop;
	connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
	loop.exec();

	QSettings s;
	s.beginGroup("WebApps");
	s.beginGroup("Facebook");

	QJsonDocument albumsDoc = QJsonDocument::fromJson(reply->readAll());
	QJsonArray albumObj = albumsDoc.object().value("data").toArray();
	foreach(const QJsonValue &v, albumObj){
		QJsonObject obj = v.toObject();
		if (obj.value("name").toString() == albumName) {
			qDebug() << "Album already Exists, using it's id";
			s.setValue("AlbumId", obj.value("id").toString());
			qDebug() << "Got album ID";
			return;
		}
	}

	qDebug() << "Album doesn't exists, creating one.";
	QUrlQuery params;
	qDebug() << "TRYING TO SET NAME" << albumName;
	params.addQueryItem("name", albumName );
	params.addQueryItem("description", "Subsurface Album");
	params.addQueryItem("privacy", "{'value': 'SELF'}");

	QNetworkRequest request(albumListUrl);
	request.setHeader(QNetworkRequest::ContentTypeHeader, "application/octet-stream");
	reply = manager->post(request, params.query().toLocal8Bit());
	connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
	loop.exec();

	albumsDoc = QJsonDocument::fromJson(reply->readAll());
	QJsonObject album = albumsDoc.object();
	if (album.contains("id")) {
		s.setValue("AlbumId", album.value("id").toString());
		qDebug() << "Got album ID";
		return;
	}

	qDebug() << "Error getting album id" << album;
}

void FacebookManager::requestUserId()
{
	qDebug() << "trying to get user Id";
	QUrl userIdRequest("https://graph.facebook.com/me?fields=id&access_token=" + QString(prefs.facebook.access_token));
	QNetworkAccessManager *getUserID = new QNetworkAccessManager();
	QNetworkReply *reply = getUserID->get(QNetworkRequest(userIdRequest));

	QEventLoop loop;
	connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
	loop.exec();

	QJsonDocument jsonDoc = QJsonDocument::fromJson(reply->readAll());
	QJsonObject obj = jsonDoc.object();
	if (obj.keys().contains("id")){
		QSettings s;
		s.beginGroup("WebApps");
		s.beginGroup("Facebook");
		s.setValue("UserId", obj.value("id").toVariant());
		qDebug() << "Got user id.";
		return;
	}
	qDebug() << "error getting user id" << obj;
}

void FacebookManager::setDesiredAlbumName(const QString& a)
{
	albumName = a;
}

/* to be changed to export the currently selected dive as shown on the profile.
 * Much much easier, and its also good to people do not select all the dives
 * and send erroniously *all* of them to facebook. */
void FacebookManager::sendDive(int divenr)
{
	ProfileWidget2 *profile = MainWindow::instance()->graphics();
	QPixmap pix = QPixmap::grabWidget(profile);

	QByteArray bytes;
	QBuffer buffer(&bytes);
	buffer.open(QIODevice::WriteOnly);
	pix.save(&buffer, "PNG");
	QUrl url("https://graph.facebook.com/v2.2/" + QString(prefs.facebook.album_id) + "/photos?" +
		 "&access_token=" + QString(prefs.facebook.access_token) +
		 "&source=image");


	QNetworkAccessManager *am = new QNetworkAccessManager(this);
	QNetworkRequest request(url);

	QString bound="margin";

	//according to rfc 1867 we need to put this string here:
	QByteArray data(QString("--" + bound + "\r\n").toLocal8Bit());
	data.append("Content-Disposition: form-data; name=\"action\"\r\n\r\n");
	data.append("https://graph.facebook.com/v2.2/\r\n");
	data.append("--" + bound + "\r\n");   //according to rfc 1867
	data.append("Content-Disposition: form-data; name=\"uploaded\"; filename=\"" + QString::number(qrand()) + ".png\"\r\n");  //name of the input is "uploaded" in my form, next one is a file name.
	data.append("Content-Type: image/jpeg\r\n\r\n"); //data type
	data.append(bytes);   //let's read the file
	data.append("\r\n");
	data.append("--" + bound + "--\r\n");  //closing boundary according to rfc 1867

	request.setRawHeader(QString("Content-Type").toLocal8Bit(),QString("multipart/form-data; boundary=" + bound).toLocal8Bit());
	request.setRawHeader(QString("Content-Length").toLocal8Bit(), QString::number(data.length()).toLocal8Bit());
	QNetworkReply *reply = am->post(request,data);

	QEventLoop loop;
	connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
	loop.exec();

	qDebug() << reply->readAll();
}
