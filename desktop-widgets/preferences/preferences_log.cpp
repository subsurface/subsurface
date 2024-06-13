// SPDX-License-Identifier: GPL-2.0
#include "preferences_log.h"
#include "ui_preferences_log.h"
#include "core/dive.h"
#include "core/settings/qPrefLog.h"
#include "core/settings/qPrefDisplay.h"
#include "core/settings/qPrefCloudStorage.h"
#include "core/settings/qPrefDiveComputer.h"
#include "core/subsurface-qt/divelistnotifier.h"

#include <QFileDialog>
#include <QProcess>
#include <QMessageBox>

PreferencesLog::PreferencesLog(): AbstractPreferencesWidget(tr(" Dive log"), QIcon(":preferences-log-icon"), 4 ), ui(new Ui::PreferencesLog())
{
	ui->setupUi(this);
}

PreferencesLog::~PreferencesLog()
{
	delete ui;
}

void PreferencesLog::on_chooseFile_clicked()
{
	QFileInfo fi(QString::fromStdString(system_default_filename()));
	QString choosenFileName = QFileDialog::getOpenFileName(this, tr("Open default log file"), fi.absolutePath(), tr("Subsurface files") + " (*.ssrf *.xml)");

	if (!choosenFileName.isEmpty())
		ui->defaultfilename->setText(choosenFileName);
}

void PreferencesLog::on_btnUseDefaultFile_toggled(bool toggle)
{
	if (toggle) {
		ui->defaultfilename->setText(QString::fromStdString(system_default_filename()));
		ui->defaultfilename->setEnabled(false);
	} else {
		ui->defaultfilename->setEnabled(true);
	}
}

void PreferencesLog::on_localDefaultFile_toggled(bool toggle)
{
	ui->defaultfilename->setEnabled(toggle);
	ui->btnUseDefaultFile->setEnabled(toggle);
	ui->chooseFile->setEnabled(toggle);
}

void PreferencesLog::refreshSettings()
{
	ui->defaultfilename->setText(qPrefLog::default_filename());
	ui->noDefaultFile->setChecked(qPrefLog::default_file_behavior() == NO_DEFAULT_FILE);
	ui->cloudDefaultFile->setChecked(qPrefLog::default_file_behavior() == CLOUD_DEFAULT_FILE);
	ui->localDefaultFile->setChecked(qPrefLog::default_file_behavior() == LOCAL_DEFAULT_FILE);

	ui->btnUseDefaultFile->setChecked(qPrefLog::use_default_file());

	if (qPrefCloudStorage::cloud_verification_status() == qPrefCloudStorage::CS_VERIFIED) {
		ui->cloudDefaultFile->setEnabled(true);
	} else {
		if (ui->cloudDefaultFile->isChecked())
			ui->noDefaultFile->setChecked(true);
		ui->cloudDefaultFile->setEnabled(false);
	}

	ui->defaultfilename->setEnabled(qPrefLog::default_file_behavior() == LOCAL_DEFAULT_FILE);
	ui->btnUseDefaultFile->setEnabled(qPrefLog::default_file_behavior() == LOCAL_DEFAULT_FILE);
	ui->chooseFile->setEnabled(qPrefLog::default_file_behavior() == LOCAL_DEFAULT_FILE);
	ui->show_average_depth->setChecked(prefs.show_average_depth);
	ui->displayinvalid->setChecked(qPrefDisplay::display_invalid_dives());
	ui->extraEnvironmentalDefault->setChecked(prefs.extraEnvironmentalDefault);
	ui->salinityEditDefault->setChecked(prefs.salinityEditDefault);
}

void PreferencesLog::syncSettings()
{
	auto log = qPrefLog::instance();
	log->set_default_filename(ui->defaultfilename->text());
	log->set_use_default_file(ui->btnUseDefaultFile->isChecked());
	if (ui->noDefaultFile->isChecked())
		log->set_default_file_behavior(NO_DEFAULT_FILE);
	else if (ui->localDefaultFile->isChecked())
		log->set_default_file_behavior(LOCAL_DEFAULT_FILE);
	else if (ui->cloudDefaultFile->isChecked())
		log->set_default_file_behavior(CLOUD_DEFAULT_FILE);

	bool displayinvalid_changed = ui->displayinvalid->isChecked() != prefs.display_invalid_dives;

	qPrefLog::set_show_average_depth(ui->show_average_depth->isChecked());
	qPrefDisplay::set_display_invalid_dives(ui->displayinvalid->isChecked());
	qPrefLog::set_extraEnvironmentalDefault(ui->extraEnvironmentalDefault->isChecked());
	qPrefLog::set_salinityEditDefault(ui->salinityEditDefault->isChecked());

	// TODO: Move to preferences code?
	if (displayinvalid_changed)
		emit diveListNotifier.filterReset();
}
