// SPDX-License-Identifier: GPL-2.0
#include "preferences_network.h"
#include "ui_preferences_network.h"
#include "subsurfacewebservices.h"
#include "core/cloudstorage.h"
#include "core/errorhelper.h"
#include "core/settings/qPrefCloudStorage.h"
#include "core/settings/qPrefProxy.h"
#include <QNetworkProxy>

PreferencesNetwork::PreferencesNetwork() : AbstractPreferencesWidget(tr("Network"),QIcon(":preferences-system-network-icon"), 9), ui(new Ui::PreferencesNetwork())
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
	updateCloudAuthenticationState();
}

void PreferencesNetwork::syncSettings()
{
	auto cloud = qPrefCloudStorage::instance();
	auto proxy = qPrefProxy::instance();

	proxy->set_proxy_type(ui->proxyType->itemData(ui->proxyType->currentIndex()).toInt());
	proxy->set_proxy_host(ui->proxyHost->text());
	proxy->set_proxy_port(ui->proxyPort->value());
	proxy->set_proxy_auth(ui->proxyAuthRequired->isChecked());
	proxy->set_proxy_user(ui->proxyUsername->text());
	proxy->set_proxy_pass(ui->proxyPassword->text());

	QString email = ui->cloud_storage_email->text().toLower();
	QString password = ui->cloud_storage_password->text();
	QString newpassword = ui->cloud_storage_new_passwd->text();

	//TODO: Change this to the Cloud Storage Stuff, not preferences.
	if (prefs.cloud_verification_status == qPrefCloudStorage::CS_VERIFIED && !newpassword.isEmpty()) {
		// deal with password change
		if (!email.isEmpty() && !password.isEmpty()) {
			// connect to backend server to check / create credentials
			QRegularExpression reg("^[a-zA-Z0-9@.+_-]+$");
			if (!reg.match(email).hasMatch() || (!password.isEmpty() && !reg.match(password).hasMatch())) {
				report_error(qPrintable(tr("Change ignored. Cloud storage email and password can only consist of letters, numbers, and '.', '-', '_', and '+'.")));
				return;
			}
			if (!reg.match(email).hasMatch() || (!newpassword.isEmpty() && !reg.match(newpassword).hasMatch())) {
				report_error(qPrintable(tr("Change ignored. Cloud storage email and new password can only consist of letters, numbers, and '.', '-', '_', and '+'.")));
				ui->cloud_storage_new_passwd->setText("");
				return;
			}
			CloudStorageAuthenticate *cloudAuth = new CloudStorageAuthenticate(this);
			connect(cloudAuth, &CloudStorageAuthenticate::finishedAuthenticate, this, &PreferencesNetwork::updateCloudAuthenticationState);
			connect(cloudAuth, &CloudStorageAuthenticate::passwordChangeSuccessful, this, &PreferencesNetwork::passwordUpdateSuccessful);
			cloudAuth->backend(email, password, "", newpassword);
			ui->cloud_storage_new_passwd->setText("");
		}
	} else if (prefs.cloud_verification_status == qPrefCloudStorage::CS_UNKNOWN ||
		   prefs.cloud_verification_status == qPrefCloudStorage::CS_INCORRECT_USER_PASSWD ||
		   email != prefs.cloud_storage_email ||
		   password != prefs.cloud_storage_password) {

		// different credentials - reset verification status
		int oldVerificationStatus = cloud->cloud_verification_status();
		cloud->set_cloud_verification_status(qPrefCloudStorage::CS_UNKNOWN);
		if (!email.isEmpty() && !password.isEmpty()) {
			// connect to backend server to check / create credentials
			QRegularExpression reg("^[a-zA-Z0-9@.+_-]+$");
			if (!reg.match(email).hasMatch() || (!password.isEmpty() && !reg.match(password).hasMatch())) {
				report_error(qPrintable(tr("Cloud storage email and password can only consist of letters, numbers, and '.', '-', '_', and '+'.")));
				cloud->set_cloud_verification_status(oldVerificationStatus);
				return;
			}
			CloudStorageAuthenticate *cloudAuth = new CloudStorageAuthenticate(this);
			connect(cloudAuth, &CloudStorageAuthenticate::finishedAuthenticate, this, &PreferencesNetwork::updateCloudAuthenticationState);
			cloudAuth->backend(email, password);
		}
	} else if (prefs.cloud_verification_status == qPrefCloudStorage::CS_NEED_TO_VERIFY) {
		QString pin = ui->cloud_storage_pin->text();
		if (!pin.isEmpty()) {
			// connect to backend server to check / create credentials
			QRegularExpression reg("^[a-zA-Z0-9@.+_-]+$");
			if (!reg.match(email).hasMatch() || !reg.match(password).hasMatch()) {
				report_error(qPrintable(tr("Cloud storage email and password can only consist of letters, numbers, and '.', '-', '_', and '+'.")));
				return;
			}
			CloudStorageAuthenticate *cloudAuth = new CloudStorageAuthenticate(this);
			connect(cloudAuth, SIGNAL(finishedAuthenticate()), this, SLOT(updateCloudAuthenticationState()));
			cloudAuth->backend(email, password, pin);
		}
	}
	cloud->set_cloud_storage_email(email);
	cloud->set_save_password_local(ui->save_password_local->isChecked());
	cloud->set_cloud_storage_password(password);
	cloud->set_cloud_verification_status(prefs.cloud_verification_status);
	cloud->set_cloud_base_url(prefs.cloud_base_url);
}

void PreferencesNetwork::updateCloudAuthenticationState()
{
	ui->cloud_storage_pin->setEnabled(prefs.cloud_verification_status == qPrefCloudStorage::CS_NEED_TO_VERIFY);
	ui->cloud_storage_pin->setVisible(prefs.cloud_verification_status == qPrefCloudStorage::CS_NEED_TO_VERIFY);
	ui->cloud_storage_pin_label->setEnabled(prefs.cloud_verification_status == qPrefCloudStorage::CS_NEED_TO_VERIFY);
	ui->cloud_storage_pin_label->setVisible(prefs.cloud_verification_status == qPrefCloudStorage::CS_NEED_TO_VERIFY);
	ui->cloud_storage_new_passwd->setEnabled(prefs.cloud_verification_status == qPrefCloudStorage::CS_VERIFIED);
	ui->cloud_storage_new_passwd->setVisible(prefs.cloud_verification_status == qPrefCloudStorage::CS_VERIFIED);
	ui->cloud_storage_new_passwd_label->setEnabled(prefs.cloud_verification_status == qPrefCloudStorage::CS_VERIFIED);
	ui->cloud_storage_new_passwd_label->setVisible(prefs.cloud_verification_status == qPrefCloudStorage::CS_VERIFIED);
	if (prefs.cloud_verification_status == qPrefCloudStorage::CS_VERIFIED) {
		ui->cloudStorageGroupBox->setTitle(tr("Subsurface cloud storage (credentials verified)"));
	} else if (prefs.cloud_verification_status == qPrefCloudStorage::CS_INCORRECT_USER_PASSWD) {
		ui->cloudStorageGroupBox->setTitle(tr("Subsurface cloud storage (incorrect password)"));
	} else if (prefs.cloud_verification_status == qPrefCloudStorage::CS_NEED_TO_VERIFY) {
		ui->cloudStorageGroupBox->setTitle(tr("Subsurface cloud storage (PIN required)"));
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

void PreferencesNetwork::passwordUpdateSuccessful()
{
	ui->cloud_storage_password->setText(prefs.cloud_storage_password);
}
