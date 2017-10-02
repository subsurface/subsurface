// SPDX-License-Identifier: GPL-2.0
#include "facebookconnectwidget.h"

#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QNetworkAccessManager>
#include <QNetworkCookieJar>

#include <QUrlQuery>
#include <QHttpMultiPart>
#include <QFile>
#include <QBuffer>
#include <QDebug>
#include <QMessageBox>
#include <QInputDialog>
#include <QLoggingCategory>
#ifdef USE_WEBENGINE
#include <QWebEngineView>
#else
#include <QWebView>
#endif
#include "mainwindow.h"
#include "profile-widget/profilewidget2.h"

#include "core/pref.h"
#include "core/helpers.h"
#include "core/subsurface-qt/SettingsObjectWrapper.h"

#include "ui_socialnetworksdialog.h"
#include "ui_facebookconnectwidget.h"

Q_LOGGING_CATEGORY(lcFacebook, "subsurface.facebook")

FacebookManager *FacebookManager::instance()
{
	static FacebookManager *self = new FacebookManager();
	return self;
}

FacebookManager::FacebookManager(QObject *parent) :
	QObject(parent),
	manager(new QNetworkAccessManager(this))
{
	connect(this, &FacebookManager::albumIdReceived, this, &FacebookManager::sendDiveToAlbum);
}

static QString graphApi = QStringLiteral("https://graph.facebook.com/v2.10/");

QUrl FacebookManager::albumListUrl()
{
	return QUrl("https://graph.facebook.com/me/albums?access_token=" + QString(prefs.facebook.access_token));
}

QUrl FacebookManager::connectUrl() {
	return QUrl("https://www.facebook.com/dialog/oauth?"
		    "client_id=427722490709000"
		    "&redirect_uri=http://www.facebook.com/connect/login_success.html"
		    "&response_type=token,granted_scopes"
		    "&display=popup"
		    "&scope=publish_actions,user_photos"
		    );
}

bool FacebookManager::loggedIn() {
	return prefs.facebook.access_token != NULL;
}

void FacebookManager::tryLogin(const QUrl& loginResponse)
{
	qCDebug(lcFacebook) << "Current url call" << loginResponse;
	QString result = loginResponse.toString();
	if (!result.contains("access_token")) {
		qCDebug(lcFacebook) << "Response without access token!";
		return;
	}

	if (result.contains("denied_scopes=publish_actions") || result.contains("denied_scopes=user_photos")) {
		qCDebug(lcFacebook) << "user did not allow us access" << result;
		return;
	}

	int from = result.indexOf("access_token=") + strlen("access_token=");
	int to = result.indexOf("&expires_in");
	QString securityToken = result.mid(from, to-from);

	auto fb = SettingsObjectWrapper::instance()->facebook;
	fb->setAccessToken(securityToken);
	qCDebug(lcFacebook) << "Got securityToken" << securityToken;
	requestUserId();
}

void FacebookManager::logout()
{
	auto fb = SettingsObjectWrapper::instance()->facebook;
	fb->setAccessToken(QString());
	fb->setUserId(QString());
	fb->setAlbumId(QString());
	emit justLoggedOut(true);
}

void FacebookManager::requestAlbumId()
{
	qCDebug(lcFacebook) << "Starting to request the album id" << albumListUrl();
	QNetworkReply *reply = manager->get(QNetworkRequest(albumListUrl()));
	connect(reply, &QNetworkReply::finished, this, &FacebookManager::albumListReceived);
}

void FacebookManager::albumListReceived()
{
	qCDebug(lcFacebook) << "Reply for the album id";
	QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
	QJsonDocument albumsDoc = QJsonDocument::fromJson(reply->readAll());
	QJsonArray albumObj = albumsDoc.object().value("data").toArray();
	auto fb = SettingsObjectWrapper::instance()->facebook;

	reply->deleteLater();
	foreach(const QJsonValue &v, albumObj){
		QJsonObject obj = v.toObject();
		if (obj.value("name").toString() == fbInfo.albumName) {
			fb->setAlbumId(obj.value("id").toString());
			qCDebug(lcFacebook) << "Album" << fbInfo.albumName << "already exists, using id" << obj.value("id").toString();
			emit albumIdReceived(fb->albumId());
			return;
		}
	}

	// No album with the name we requested, create a new one.
	createFacebookAlbum();
}

void FacebookManager::createFacebookAlbum()
{
	qCDebug(lcFacebook) << "Album with name" << fbInfo.albumName << "doesn't exists, creating it.";
	QUrlQuery params;
	params.addQueryItem("name", fbInfo.albumName );
	params.addQueryItem("description", "Subsurface Album");
	params.addQueryItem("privacy", "{'value': 'SELF'}");

	QNetworkRequest request(albumListUrl());
	request.setHeader(QNetworkRequest::ContentTypeHeader, "application/octet-stream");

	QNetworkReply *reply = manager->post(request, params.query().toLocal8Bit());
	connect(reply, &QNetworkReply::finished, this, &FacebookManager::facebookAlbumCreated);
}

void FacebookManager::facebookAlbumCreated()
{
	QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
	QJsonDocument albumsDoc = QJsonDocument::fromJson(reply->readAll());
	QJsonObject album = albumsDoc.object();

	reply->deleteLater();

	if (album.contains("id")) {
		qCDebug(lcFacebook) << "Album" << fbInfo.albumName << "created successfully with id" << album.value("id").toString();
		auto fb = SettingsObjectWrapper::instance()->facebook;
		fb->setAlbumId(album.value("id").toString());
		emit albumIdReceived(fb->albumId());
		return;
	} else {
		qCDebug(lcFacebook) << "It was not possible to create the album with name" << fbInfo.albumName;
	}
}

void FacebookManager::requestUserId()
{
	qCDebug(lcFacebook) << "Requesting user id";
	QUrl userIdRequest("https://graph.facebook.com/me?fields=id&access_token=" + QString(prefs.facebook.access_token));
	QNetworkReply *reply = manager->get(QNetworkRequest(userIdRequest));

	connect(reply, &QNetworkReply::finished, this, &FacebookManager::userIdReceived);
}

void FacebookManager::userIdReceived()
{
	QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
	QJsonDocument jsonDoc = QJsonDocument::fromJson(reply->readAll());
	QJsonObject obj = jsonDoc.object();
	if (obj.keys().contains("id")) {
		qCDebug(lcFacebook) << "User id requested successfully:" << obj.value("id").toString();
		SettingsObjectWrapper::instance()->facebook->setUserId(obj.value("id").toString());
		emit justLoggedIn(true);
	} else {
		qCDebug(lcFacebook) << "Error, user id unknown, cannot login.";
	}
	reply->deleteLater();
}

QPixmap FacebookManager::grabProfilePixmap()
{
	qCDebug(lcFacebook) << "Grabbing Dive Profile pixmap";
	ProfileWidget2 *profile = MainWindow::instance()->graphics();

	QSize size = fbInfo.profileSize == FacebookInfo::SMALL  ? QSize(800,600) :
		     fbInfo.profileSize == FacebookInfo::MEDIUM ? QSize(1024,760) :
		     fbInfo.profileSize == FacebookInfo::BIG ? QSize(1280,1024) : QSize();

	auto currSize = profile->size();
	profile->resize(size);
	profile->setToolTipVisibile(false);
	QPixmap pix = profile->grab();
	profile->setToolTipVisibile(true);
	profile->resize(currSize);

	return pix;
}


/* to be changed to export the currently selected dive as shown on the profile.
 * Much much easier, and its also good to people do not select all the dives
 * and send erroniously *all* of them to facebook. */
void FacebookManager::sendDiveInit()
{
	qCDebug(lcFacebook) << "Starting to upload the dive to facebook";

	SocialNetworkDialog dialog(qApp->activeWindow());
	if (dialog.exec() != QDialog::Accepted) {
		qCDebug(lcFacebook) << "User cancelled.";
		return;
	}

	fbInfo.bodyText = dialog.text();
	fbInfo.profileSize = dialog.profileSize();
	fbInfo.profileData = grabProfilePixmap();
	fbInfo.albumName = dialog.album();
	fbInfo.albumId = QString(); // request Album Id wil handle that.

	// will emit albumIdReceived, that's connected to sendDiveToAlbum
	requestAlbumId();
}

void FacebookManager::sendDiveToAlbum(const QString& albumId)
{
	qCDebug(lcFacebook) << "Starting to upload the dive to album" << fbInfo.albumName << "id" << albumId;
	QUrl url(graphApi + albumId + "/photos?" +
		 "&access_token=" + QString(prefs.facebook.access_token) +
		 "&source=image" +
		 "&message=" + fbInfo.bodyText.replace("&quot;", "%22"));

	QNetworkRequest request(url);

	QString bound="margin";

	//according to rfc 1867 we need to put this string here:
	QByteArray data(QString("--" + bound + "\r\n").toLocal8Bit());
	data.append("Content-Disposition: form-data; name=\"action\"\r\n\r\n");
	data.append(graphApi + "\r\n");
	data.append("--" + bound + "\r\n");   //according to rfc 1867

	//name of the input is "uploaded" in my form, next one is a file name.
	data.append("Content-Disposition: form-data; name=\"uploaded\"; filename=\"" + QString::number(qrand()) + ".png\"\r\n");
	data.append("Content-Type: image/jpeg\r\n\r\n"); //data type

	QByteArray bytes;
	QBuffer buffer(&bytes);
	buffer.open(QIODevice::WriteOnly);
	fbInfo.profileData.save(&buffer, "PNG");

	data.append(bytes);   //let's read the file
	data.append("\r\n");
	data.append("--" + bound + "--\r\n");  //closing boundary according to rfc 1867

	request.setRawHeader(QByteArray("Content-Type"),QString("multipart/form-data; boundary=" + bound).toLocal8Bit());
	request.setRawHeader(QByteArray("Content-Length"), QString::number(data.length()).toLocal8Bit());
	QNetworkReply *reply = manager->post(request,data);

	connect(reply, &QNetworkReply::finished, this, &FacebookManager::uploadFinished);
}

void FacebookManager::uploadFinished()
{
	qCDebug(lcFacebook) << "Upload finish";
	auto reply = qobject_cast<QNetworkReply*>(sender());
	QByteArray response = reply->readAll();
	QJsonDocument jsonDoc = QJsonDocument::fromJson(response);
	QJsonObject obj = jsonDoc.object();

	reply->deleteLater();

	if (obj.keys().contains("id")){
		QMessageBox::information(qApp->activeWindow(),
					 tr("Photo upload sucessfull"),
					 tr("Your dive profile was updated to Facebook."),
		QMessageBox::Ok);
	} else {
		QMessageBox::information(qApp->activeWindow(),
					 tr("Photo upload failed"),
					 tr("Your dive profile was not updated to Facebook, \n "
					    "please send the following to the developer. \n"
					    + response),
		QMessageBox::Ok);
	}

	emit sendDiveFinished();
}

FacebookConnectWidget::FacebookConnectWidget(QWidget *parent) : QDialog(parent), ui(new Ui::FacebookConnectWidget) {
	ui->setupUi(this);
	FacebookManager *fb = FacebookManager::instance();
#ifdef USE_WEBENGINE
	facebookWebView = new QWebEngineView(this);
#else
	facebookWebView = new QWebView(this);
#endif
	ui->fbWebviewContainer->layout()->addWidget(facebookWebView);
	if (fb->loggedIn()) {
		facebookLoggedIn();
	} else {
		facebookDisconnect();
	}
#ifdef USE_WEBENGINE
	connect(facebookWebView, &QWebEngineView::urlChanged, fb, &FacebookManager::tryLogin);
#else
	connect(facebookWebView, &QWebView::urlChanged, fb, &FacebookManager::tryLogin);
#endif
	connect(fb, &FacebookManager::justLoggedIn, this, &FacebookConnectWidget::facebookLoggedIn);
	connect(fb, &FacebookManager::justLoggedOut, this, &FacebookConnectWidget::facebookDisconnect);
}

void FacebookConnectWidget::facebookLoggedIn()
{
	ui->fbWebviewContainer->hide();
	ui->fbWebviewContainer->setEnabled(false);
	ui->FBLabel->setText(tr("To disconnect Subsurface from your Facebook account, use the 'Share on' menu entry."));
}

void FacebookConnectWidget::facebookDisconnect()
{
	qCDebug(lcFacebook) << "Disconnecting from facebook";
	// remove the connect/disconnect button
	// and instead add the login view
	ui->fbWebviewContainer->show();
	ui->fbWebviewContainer->setEnabled(true);
	ui->FBLabel->setText(tr("To connect to Facebook, please log in. This enables Subsurface to publish dives to your timeline"));
	if (facebookWebView) {
#ifdef USE_WEBENGINE
	//FIX ME
#else
		facebookWebView->page()->networkAccessManager()->setCookieJar(new QNetworkCookieJar());
#endif
		facebookWebView->setUrl(FacebookManager::instance()->connectUrl());
	}
}

SocialNetworkDialog::SocialNetworkDialog(QWidget *parent) :
	QDialog(parent),
	ui( new Ui::SocialnetworksDialog())
{
	ui->setupUi(this);
	ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
	connect(ui->date, &QCheckBox::clicked, this, &SocialNetworkDialog::selectionChanged);
	connect(ui->duration, &QCheckBox::clicked, this, &SocialNetworkDialog::selectionChanged);
	connect(ui->Buddy, &QCheckBox::clicked, this, &SocialNetworkDialog::selectionChanged);
	connect(ui->Divemaster, &QCheckBox::clicked, this, &SocialNetworkDialog::selectionChanged);
	connect(ui->Location, &QCheckBox::clicked, this, &SocialNetworkDialog::selectionChanged);
	connect(ui->Notes, &QCheckBox::clicked, this, &SocialNetworkDialog::selectionChanged);
	connect(ui->album, &QLineEdit::editingFinished, this, &SocialNetworkDialog::albumChanged);
}

FacebookInfo::Size SocialNetworkDialog::profileSize() const
{
	QString currText = ui->profileSize->currentText();
	return currText.startsWith(tr("Small")) ? FacebookInfo::SMALL :
	       currText.startsWith(tr("Medium")) ?  FacebookInfo::MEDIUM :
	       /* currText.startsWith(tr("Big")) ? */ FacebookInfo::BIG;
}


void SocialNetworkDialog::albumChanged()
{
	QAbstractButton *button = ui->buttonBox->button(QDialogButtonBox::Ok);
	button->setEnabled(!ui->album->text().isEmpty());
}

void SocialNetworkDialog::selectionChanged()
{
	struct dive *d = current_dive;
	QString fullText;

	if (!d)
		return;

	if (ui->date->isChecked()) {
		fullText += tr("Dive date: %1 \n").arg(get_short_dive_date_string(d->when));
	}
	if (ui->duration->isChecked()) {
		fullText += tr("Duration: %1 \n").arg(get_dive_duration_string(d->duration.seconds,
									       tr("h", "abbreviation for hours"),
									       tr("min", "abbreviation for minutes")));
	}
	if (ui->Location->isChecked()) {
		fullText += tr("Dive location: %1 \n").arg(get_dive_location(d));
	}
	if (ui->Buddy->isChecked()) {
		fullText += tr("Buddy: %1 \n").arg(d->buddy);
	}
	if (ui->Divemaster->isChecked()) {
		fullText += tr("Divemaster: %1 \n").arg(d->divemaster);
	}
	if (ui->Notes->isChecked()) {
		fullText += tr("\n%1").arg(d->notes);
	}
	ui->text->setPlainText(fullText);
}

QString SocialNetworkDialog::text() const {
	return ui->text->toPlainText().toHtmlEscaped();
}

QString SocialNetworkDialog::album() const {
	return ui->album->text().toHtmlEscaped();
}


