// SPDX-License-Identifier: GPL-2.0
#include "preferences_cloud.h"
#include "ui_preferences_cloud.h"
#include "subsurfacewebservices.h"
#include "core/cloudstorage.h"
#include "core/errorhelper.h"
#include "core/settings/qPrefCloudStorage.h"
#include <QRegularExpression>
#include <QMessageBox>
#include <QDesktopServices>

PreferencesCloud::PreferencesCloud() : AbstractPreferencesWidget(tr("Cloud"),QIcon(":preferences-cloud-icon"), 9), ui(new Ui::PreferencesCloud())
{
	ui->setupUi(this);

	ui->label_help2->setWordWrap(true);
	ui->label_help3->setWordWrap(true);
	ui->label_help4->setWordWrap(true);
}

PreferencesCloud::~PreferencesCloud()
{
	delete ui;
}

void PreferencesCloud::on_resetPassword_clicked()
{
	QDesktopServices::openUrl(QUrl("https://cloud.subsurface-divelog.org/passwordreset"));
}

void PreferencesCloud::refreshSettings()
{
	ui->cloud_storage_email->setText(prefs.cloud_storage_email);
	ui->cloud_storage_password->setText(prefs.cloud_storage_password);
	ui->save_password_local->setChecked(prefs.save_password_local);
	updateCloudAuthenticationState();
}

void PreferencesCloud::syncSettings()
{
	auto cloud = qPrefCloudStorage::instance();

	QString email = ui->cloud_storage_email->text().toLower();
	QString password = ui->cloud_storage_password->text();
	QString newpassword = ui->cloud_storage_new_passwd->text();
	QString emailpasswordformatwarning = tr("Change ignored. Cloud storage email and new password can only consist of letters, numbers, and '.', '-', '_', and '+'.");

	//TODO: Change this to the Cloud Storage Stuff, not preferences.
	if (prefs.cloud_verification_status == qPrefCloudStorage::CS_VERIFIED && !newpassword.isEmpty()) {
		// deal with password change
		if (!email.isEmpty() && !password.isEmpty()) {
			// connect to backend server to check / create credentials
			QRegularExpression reg("^[a-zA-Z0-9@.+_-]+$");
			if (!reg.match(email).hasMatch() || (!password.isEmpty() && !reg.match(password).hasMatch())) {
				QMessageBox::warning(this, tr("Warning"), emailpasswordformatwarning);
				return;
			}
			if (!reg.match(email).hasMatch() || (!newpassword.isEmpty() && !reg.match(newpassword).hasMatch())) {
				QMessageBox::warning(this, tr("Warning"), emailpasswordformatwarning);
				ui->cloud_storage_new_passwd->setText("");
				return;
			}
			CloudStorageAuthenticate *cloudAuth = new CloudStorageAuthenticate(this);
			connect(cloudAuth, &CloudStorageAuthenticate::finishedAuthenticate, this, &PreferencesCloud::updateCloudAuthenticationState);
			connect(cloudAuth, &CloudStorageAuthenticate::passwordChangeSuccessful, this, &PreferencesCloud::passwordUpdateSuccessful);
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
				QMessageBox::warning(this, tr("Warning"), emailpasswordformatwarning);
				cloud->set_cloud_verification_status(oldVerificationStatus);
				return;
			}
			CloudStorageAuthenticate *cloudAuth = new CloudStorageAuthenticate(this);
			connect(cloudAuth, &CloudStorageAuthenticate::finishedAuthenticate, this, &PreferencesCloud::updateCloudAuthenticationState);
			cloudAuth->backend(email, password);
		}
	} else if (prefs.cloud_verification_status == qPrefCloudStorage::CS_NEED_TO_VERIFY) {
		QString pin = ui->cloud_storage_pin->text();
		if (!pin.isEmpty()) {
			// connect to backend server to check / create credentials
			QRegularExpression reg("^[a-zA-Z0-9@.+_-]+$");
			if (!reg.match(email).hasMatch() || !reg.match(password).hasMatch()) {
				QMessageBox::warning(this, tr("Warning"), emailpasswordformatwarning);
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

void PreferencesCloud::updateCloudAuthenticationState()
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

void PreferencesCloud::passwordUpdateSuccessful()
{
	ui->cloud_storage_password->setText(prefs.cloud_storage_password);
}
