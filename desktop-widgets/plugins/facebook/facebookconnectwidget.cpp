#include "facebookconnectwidget.h"

#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QNetworkAccessManager>
#include <QNetworkCookieJar>

#include <QUrlQuery>
#include <QEventLoop>
#include <QHttpMultiPart>
#include <QSettings>
#include <QFile>
#include <QBuffer>
#include <QDebug>
#include <QMessageBox>
#include <QInputDialog>
#include <QWebView>

#include "mainwindow.h"
#include "profile-widget/profilewidget2.h"
#include "pref.h"
#include "helpers.h"
#include "ui_socialnetworksdialog.h"
#include "ui_facebookconnectwidget.h"

#if SAVE_FB_CREDENTIALS
#define GET_TXT(name, field)                                             \
	v = s.value(QString(name));                                      \
	if (v.isValid())                                                 \
		prefs.field = strdup(v.toString().toUtf8().constData()); \
	else                                                             \
		prefs.field = default_prefs.field
#endif

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
		"&response_type=token,granted_scopes"
		"&display=popup"
		"&scope=publish_actions,user_photos"
	);
}

bool FacebookManager::loggedIn() {
	return prefs.facebook.access_token != NULL;
}

void FacebookManager::sync()
{
#if SAVE_FB_CREDENTIALS
	QSettings s;
	s.beginGroup("WebApps");
	s.beginGroup("Facebook");

	QVariant v;
	GET_TXT("ConnectToken", facebook.access_token);
	GET_TXT("UserId", facebook.user_id);
	GET_TXT("AlbumId", facebook.album_id);
#endif
}

void FacebookManager::tryLogin(const QUrl& loginResponse)
{
	QString result = loginResponse.toString();
	if (!result.contains("access_token"))
		return;

	if (result.contains("denied_scopes=publish_actions") || result.contains("denied_scopes=user_photos")) {
		qDebug() << "user did not allow us access" << result;
		return;
	}
	int from = result.indexOf("access_token=") + strlen("access_token=");
	int to = result.indexOf("&expires_in");
	QString securityToken = result.mid(from, to-from);

#if SAVE_FB_CREDENTIALS
	QSettings settings;
	settings.beginGroup("WebApps");
	settings.beginGroup("Facebook");
	settings.setValue("ConnectToken", securityToken);
	sync();
#else
	prefs.facebook.access_token = copy_string(securityToken.toUtf8().data());
#endif
	requestUserId();
	sync();
	emit justLoggedIn(true);
}

void FacebookManager::logout()
{
#if SAVE_FB_CREDENTIALS
	QSettings settings;
	settings.beginGroup("WebApps");
	settings.beginGroup("Facebook");
	settings.remove("ConnectToken");
	settings.remove("UserId");
	settings.remove("AlbumId");
	sync();
#else
	free(prefs.facebook.access_token);
	free(prefs.facebook.album_id);
	free(prefs.facebook.user_id);
	prefs.facebook.access_token = NULL;
	prefs.facebook.album_id = NULL;
	prefs.facebook.user_id = NULL;
#endif
	emit justLoggedOut(true);
}

void FacebookManager::requestAlbumId()
{
	QUrl albumListUrl("https://graph.facebook.com/me/albums?access_token=" + QString(prefs.facebook.access_token));
	QNetworkAccessManager *manager = new QNetworkAccessManager();
	QNetworkReply *reply = manager->get(QNetworkRequest(albumListUrl));

	QEventLoop loop;
	connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
	loop.exec();

#if SAVE_FB_CREDENTIALS
	QSettings s;
	s.beginGroup("WebApps");
	s.beginGroup("Facebook");
#endif

	QJsonDocument albumsDoc = QJsonDocument::fromJson(reply->readAll());
	QJsonArray albumObj = albumsDoc.object().value("data").toArray();
	foreach(const QJsonValue &v, albumObj){
		QJsonObject obj = v.toObject();
		if (obj.value("name").toString() == albumName) {
#if SAVE_FB_CREDENTIALS
			s.setValue("AlbumId", obj.value("id").toString());
#else
			prefs.facebook.album_id = copy_string(obj.value("id").toString().toUtf8().data());
#endif
			return;
		}
	}

	QUrlQuery params;
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
#if SAVE_FB_CREDENTIALS
		s.setValue("AlbumId", album.value("id").toString());
#else
		prefs.facebook.album_id = copy_string(album.value("id").toString().toUtf8().data());
#endif
		sync();
		return;
	}
}

void FacebookManager::requestUserId()
{
	QUrl userIdRequest("https://graph.facebook.com/me?fields=id&access_token=" + QString(prefs.facebook.access_token));
	QNetworkAccessManager *getUserID = new QNetworkAccessManager();
	QNetworkReply *reply = getUserID->get(QNetworkRequest(userIdRequest));

	QEventLoop loop;
	connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
	loop.exec();

	QJsonDocument jsonDoc = QJsonDocument::fromJson(reply->readAll());
	QJsonObject obj = jsonDoc.object();
	if (obj.keys().contains("id")){
#if SAVE_FB_CREDENTIALS
		QSettings s;
		s.beginGroup("WebApps");
		s.beginGroup("Facebook");
		s.setValue("UserId", obj.value("id").toVariant());
#else
		prefs.facebook.user_id = copy_string(obj.value("id").toString().toUtf8().data());
#endif
		return;
	}
}

void FacebookManager::setDesiredAlbumName(const QString& a)
{
	albumName = a;
}

/* to be changed to export the currently selected dive as shown on the profile.
 * Much much easier, and its also good to people do not select all the dives
 * and send erroniously *all* of them to facebook. */
void FacebookManager::sendDive()
{
	SocialNetworkDialog dialog(qApp->activeWindow());
	if (dialog.exec() != QDialog::Accepted)
		return;

	setDesiredAlbumName(dialog.album());
	requestAlbumId();

	ProfileWidget2 *profile = MainWindow::instance()->graphics();
	profile->setToolTipVisibile(false);
	QPixmap pix = QPixmap::grabWidget(profile);
	profile->setToolTipVisibile(true);
	QByteArray bytes;
	QBuffer buffer(&bytes);
	buffer.open(QIODevice::WriteOnly);
	pix.save(&buffer, "PNG");
	QUrl url("https://graph.facebook.com/v2.2/" + QString(prefs.facebook.album_id) + "/photos?" +
		 "&access_token=" + QString(prefs.facebook.access_token) +
		 "&source=image" +
		 "&message=" + dialog.text().replace("&quot;", "%22"));

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

	QByteArray response = reply->readAll();
	QJsonDocument jsonDoc = QJsonDocument::fromJson(response);
	QJsonObject obj = jsonDoc.object();
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
}

FacebookConnectWidget::FacebookConnectWidget(QWidget *parent) : QDialog(parent), ui(new Ui::FacebookConnectWidget) {
	ui->setupUi(this);
	FacebookManager *fb = FacebookManager::instance();
	facebookWebView = new QWebView(this);
	ui->fbWebviewContainer->layout()->addWidget(facebookWebView);
	if (fb->loggedIn()) {
		facebookLoggedIn();
	} else {
		facebookDisconnect();
	}
	connect(facebookWebView, &QWebView::urlChanged, fb, &FacebookManager::tryLogin);
	connect(fb, &FacebookManager::justLoggedIn, this, &FacebookConnectWidget::facebookLoggedIn);
}

void FacebookConnectWidget::facebookLoggedIn()
{
	ui->fbWebviewContainer->hide();
	ui->fbWebviewContainer->setEnabled(false);
	ui->FBLabel->setText(tr("To disconnect Subsurface from your Facebook account, use the button below"));
}

void FacebookConnectWidget::facebookDisconnect()
{
	// remove the connect/disconnect button
	// and instead add the login view
	ui->fbWebviewContainer->show();
	ui->fbWebviewContainer->setEnabled(true);
	ui->FBLabel->setText(tr("To connect to Facebook, please log in. This enables Subsurface to publish dives to your timeline"));
	if (facebookWebView) {
		facebookWebView->page()->networkAccessManager()->setCookieJar(new QNetworkCookieJar());
		facebookWebView->setUrl(FacebookManager::instance()->connectUrl());
	}
}

SocialNetworkDialog::SocialNetworkDialog(QWidget *parent) :
	QDialog(parent),
	ui( new Ui::SocialnetworksDialog())
{
	ui->setupUi(this);
	ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
	connect(ui->date, SIGNAL(clicked()), this, SLOT(selectionChanged()));
	connect(ui->duration, SIGNAL(clicked()), this, SLOT(selectionChanged()));
	connect(ui->Buddy, SIGNAL(clicked()), this, SLOT(selectionChanged()));
	connect(ui->Divemaster, SIGNAL(clicked()), this, SLOT(selectionChanged()));
	connect(ui->Location, SIGNAL(clicked()), this, SLOT(selectionChanged()));
	connect(ui->Notes, SIGNAL(clicked()), this, SLOT(selectionChanged()));
	connect(ui->album, SIGNAL(textChanged(QString)), this, SLOT(albumChanged()));
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
	if (ui->date->isChecked()) {
		fullText += tr("Dive date: %1 \n").arg(get_short_dive_date_string(d->when));
	}
	if (ui->duration->isChecked()) {
		fullText += tr("Duration: %1 \n").arg(get_dive_duration_string(d->duration.seconds,
									       tr("h:", "abbreviation for hours plus separator"),
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


