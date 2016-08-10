#include "preferences_network.h"
#include "ui_preferences_network.h"
#include "core/dive.h"
#include "subsurfacewebservices.h"
#include "core/prefs-macros.h"
#include "core/cloudstorage.h"
#include "core/subsurface-qt/SettingsObjectWrapper.h"
#include <QNetworkProxy>
#include <QSettings>

PreferencesNetwork::PreferencesNetwork() : AbstractPreferencesWidget(tr("Network"),QIcon(":network"), 9), ui(new Ui::PreferencesNetwork())
{
	ui->setupUi(this);

	ui->proxyType->clear();
	ui->proxyType->addItem(tr("No proxy"), QNetworkProxy::NoProxy);
	ui->proxyType->addItem(tr("System proxy"), QNetworkProxy::DefaultProxy);
	ui->proxyType->addItem(tr("HTTP proxy"), QNetworkProxy::HttpProxy);
	ui->proxyType->addItem(tr("SOCKS proxy"), QNetworkProxy::Socks5Proxy);
	ui->proxyType->setCurrentIndex(-1);

	connect(ui->proxyType, SIGNAL(currentIndexChanged(int)), this, SLOT(proxyType_changed(int)));
}

PreferencesNetwork::~PreferencesNetwork()
{
	delete ui;
}

void PreferencesNetwork::refreshSettings()
{
	ui->proxyHost->setText(prefs.proxy_host);
	ui->proxyPort->setValue(prefs.proxy_port);
	ui->proxyAuthRequired->setChecked(prefs.proxy_auth);
	ui->proxyUsername->setText(prefs.proxy_user);
	ui->proxyPassword->setText(prefs.proxy_pass);
	ui->proxyType->setCurrentIndex(ui->proxyType->findData(prefs.proxy_type));
	ui->cloud_storage_email->setText(prefs.cloud_storage_email);
	ui->cloud_storage_password->setText(prefs.cloud_storage_password);
	ui->save_password_local->setChecked(prefs.save_password_local);
	ui->cloud_background_sync->setChecked(prefs.cloud_background_sync);
	ui->save_uid_local->setChecked(prefs.save_userid_local);
	ui->default_uid->setText(QString(prefs.userid).toUpper());
	cloudPinNeeded();
}

void PreferencesNetwork::syncSettings()
{
	auto cloud = SettingsObjectWrapper::instance()->cloud_storage;
	auto proxy = SettingsObjectWrapper::instance()->proxy;

	cloud->setUserId(ui->default_uid->text().toUpper());
	cloud->setSaveUserIdLocal(ui->save_uid_local->checkState());

	proxy->setType(ui->proxyType->itemData(ui->proxyType->currentIndex()).toInt());
	proxy->setHost(ui->proxyHost->text());
	proxy->setPort(ui->proxyPort->value());
	proxy->setAuth(ui->proxyAuthRequired->isChecked());
	proxy->setUser(ui->proxyUsername->text());
	proxy->setPass(ui->proxyPassword->text());

	QString email = ui->cloud_storage_email->text();
	QString password = ui->cloud_storage_password->text();
	QString newpassword = ui->cloud_storage_new_passwd->text();

	//TODO: Change this to the Cloud Storage Stuff, not preferences.
	if (prefs.cloud_verification_status == CS_VERIFIED && !newpassword.isEmpty()) {
		// deal with password change
		if (!email.isEmpty() && !password.isEmpty()) {
			// connect to backend server to check / create credentials
			QRegularExpression reg("^[a-zA-Z0-9@.+_-]+$");
			if (!reg.match(email).hasMatch() || (!password.isEmpty() && !reg.match(password).hasMatch())) {
				report_error(qPrintable(tr("Cloud storage email and password can only consist of letters, numbers, and '.', '-', '_', and '+'.")));
			} else {
				CloudStorageAuthenticate *cloudAuth = new CloudStorageAuthenticate(this);
				connect(cloudAuth, SIGNAL(finishedAuthenticate()), this, SLOT(cloudPinNeeded()));
				connect(cloudAuth, SIGNAL(passwordChangeSuccessful()), this, SLOT(passwordUpdateSuccessfull()));
				cloudAuth->backend(email, password, "", newpassword);
				ui->cloud_storage_new_passwd->setText("");
				cloud->setNewPassword(newpassword);
			}
		}
	} else if (prefs.cloud_verification_status == CS_UNKNOWN ||
		   prefs.cloud_verification_status == CS_INCORRECT_USER_PASSWD ||
		   email != prefs.cloud_storage_email ||
		   password != prefs.cloud_storage_password) {

		// different credentials - reset verification status
		cloud->setVerificationStatus(CS_UNKNOWN);
		if (!email.isEmpty() && !password.isEmpty()) {
			// connect to backend server to check / create credentials
			QRegularExpression reg("^[a-zA-Z0-9@.+_-]+$");
			if (!reg.match(email).hasMatch() || (!password.isEmpty() && !reg.match(password).hasMatch())) {
				report_error(qPrintable(tr("Cloud storage email and password can only consist of letters, numbers, and '.', '-', '_', and '+'.")));
			} else {
				CloudStorageAuthenticate *cloudAuth = new CloudStorageAuthenticate(this);
				connect(cloudAuth, &CloudStorageAuthenticate::finishedAuthenticate, this, &PreferencesNetwork::cloudPinNeeded);
				cloudAuth->backend(email, password);
			}
		}
	} else if (prefs.cloud_verification_status == CS_NEED_TO_VERIFY) {
		QString pin = ui->cloud_storage_pin->text();
		if (!pin.isEmpty()) {
			// connect to backend server to check / create credentials
			QRegularExpression reg("^[a-zA-Z0-9@.+_-]+$");
			if (!reg.match(email).hasMatch() || !reg.match(password).hasMatch()) {
				report_error(qPrintable(tr("Cloud storage email and password can only consist of letters, numbers, and '.', '-', '_', and '+'.")));
			}
			CloudStorageAuthenticate *cloudAuth = new CloudStorageAuthenticate(this);
			connect(cloudAuth, SIGNAL(finishedAuthenticate()), this, SLOT(cloudPinNeeded()));
			cloudAuth->backend(email, password, pin);
		}
	}
	cloud->setEmail(email);
	cloud->setSavePasswordLocal(ui->save_password_local->isChecked());
	cloud->setPassword(password);
	cloud->setVerificationStatus(prefs.cloud_verification_status);
	cloud->setBackgroundSync(ui->cloud_background_sync->isChecked());
	cloud->setBaseUrl(prefs.cloud_base_url);
}

void PreferencesNetwork::cloudPinNeeded()
{
	ui->cloud_storage_pin->setEnabled(prefs.cloud_verification_status == CS_NEED_TO_VERIFY);
	ui->cloud_storage_pin->setVisible(prefs.cloud_verification_status == CS_NEED_TO_VERIFY);
	ui->cloud_storage_pin_label->setEnabled(prefs.cloud_verification_status == CS_NEED_TO_VERIFY);
	ui->cloud_storage_pin_label->setVisible(prefs.cloud_verification_status == CS_NEED_TO_VERIFY);
	ui->cloud_storage_new_passwd->setEnabled(prefs.cloud_verification_status == CS_VERIFIED);
	ui->cloud_storage_new_passwd->setVisible(prefs.cloud_verification_status == CS_VERIFIED);
	ui->cloud_storage_new_passwd_label->setEnabled(prefs.cloud_verification_status == CS_VERIFIED);
	ui->cloud_storage_new_passwd_label->setVisible(prefs.cloud_verification_status == CS_VERIFIED);
	if (prefs.cloud_verification_status == CS_VERIFIED) {
		ui->cloudStorageGroupBox->setTitle(tr("Subsurface cloud storage (credentials verified)"));
	} else {
		ui->cloudStorageGroupBox->setTitle(tr("Subsurface cloud storage"));
	}
	emit settingsChanged();
}

void PreferencesNetwork::proxyType_changed(int idx)
{
	if (idx == -1) {
		return;
	}

	int proxyType = ui->proxyType->itemData(idx).toInt();
	bool hpEnabled = (proxyType == QNetworkProxy::Socks5Proxy || proxyType == QNetworkProxy::HttpProxy);
	ui->proxyHost->setEnabled(hpEnabled);
	ui->proxyPort->setEnabled(hpEnabled);
	ui->proxyAuthRequired->setEnabled(hpEnabled);
	ui->proxyUsername->setEnabled(hpEnabled & ui->proxyAuthRequired->isChecked());
	ui->proxyPassword->setEnabled(hpEnabled & ui->proxyAuthRequired->isChecked());
	ui->proxyAuthRequired->setChecked(ui->proxyAuthRequired->isChecked());
}

void PreferencesNetwork::passwordUpdateSuccessfull()
{
	ui->cloud_storage_password->setText(prefs.cloud_storage_password);
}
