#include "preferences_network.h"
#include "ui_preferences_network.h"
#include "dive.h"
#include "subsurfacewebservices.h"
#include "subsurface-core/prefs-macros.h"

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
	QSettings s;

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
	ui->default_uid->setText(s.value("subsurface_webservice_uid").toString().toUpper());

	cloudPinNeeded();
}

void PreferencesNetwork::syncSettings()
{
	QSettings s;
	s.setValue("subsurface_webservice_uid", ui->default_uid->text().toUpper());
	set_save_userid_local(ui->save_uid_local->checkState());

	s.beginGroup("Network");
	s.setValue("proxy_type", ui->proxyType->itemData(ui->proxyType->currentIndex()).toInt());
	s.setValue("proxy_host", ui->proxyHost->text());
	s.setValue("proxy_port", ui->proxyPort->value());
	SB("proxy_auth", ui->proxyAuthRequired);
	s.setValue("proxy_user", ui->proxyUsername->text());
	s.setValue("proxy_pass", ui->proxyPassword->text());
	s.endGroup();

	s.beginGroup("CloudStorage");
	QString email = ui->cloud_storage_email->text();
	QString password = ui->cloud_storage_password->text();
	QString newpassword = ui->cloud_storage_new_passwd->text();
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
				free(prefs.cloud_storage_newpassword);
				prefs.cloud_storage_newpassword = strdup(qPrintable(newpassword));
			}
		}
	} else if (prefs.cloud_verification_status == CS_UNKNOWN ||
		   prefs.cloud_verification_status == CS_INCORRECT_USER_PASSWD ||
		   email != prefs.cloud_storage_email ||
		   password != prefs.cloud_storage_password) {

		// different credentials - reset verification status
		prefs.cloud_verification_status = CS_UNKNOWN;
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
	SAVE_OR_REMOVE("email", default_prefs.cloud_storage_email, email);
	SAVE_OR_REMOVE("save_password_local", default_prefs.save_password_local, ui->save_password_local->isChecked());
	if (ui->save_password_local->isChecked()) {
		SAVE_OR_REMOVE("password", default_prefs.cloud_storage_password, password);
	} else {
		s.remove("password");
		free(prefs.cloud_storage_password);
		prefs.cloud_storage_password = strdup(qPrintable(password));
	}
	SAVE_OR_REMOVE("cloud_verification_status", default_prefs.cloud_verification_status, prefs.cloud_verification_status);
	SAVE_OR_REMOVE("cloud_background_sync", default_prefs.cloud_background_sync, ui->cloud_background_sync->isChecked());

	// at this point we intentionally do not have a UI for changing this
	// it could go into some sort of "advanced setup" or something
	SAVE_OR_REMOVE("cloud_base_url", default_prefs.cloud_base_url, prefs.cloud_base_url);
	s.endGroup();
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
