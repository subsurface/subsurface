// SPDX-License-Identifier: GPL-2.0
#include "preferences_reset.h"
#include "ui_preferences_reset.h"
#include "core/dive.h"
#include "preferencesdialog.h"
//#include "core/settings/qPrefGeneral.h"
//#include "core/settings/qPrefDisplay.h"
//#include "core/settings/qPrefCloudStorage.h"
//#include "core/settings/qPrefDiveComputer.h"

#include <QFileDialog>
#include <QProcess>
#include <QMessageBox>
#include <stdlib.h>
#include <stdio.h>


PreferencesReset::PreferencesReset(): AbstractPreferencesWidget(tr("Reset"), QIcon(":preferences-reset-icon"), 11 ), ui(new Ui::PreferencesReset())
{
	ui->setupUi(this);

	int h = 2 * ui->resetWarningIcon->height();
	QPixmap warning (":preferences-reset-warning-icon");
	ui->resetWarningIcon->setPixmap(warning.scaled(h,h,Qt::KeepAspectRatio));
}

PreferencesReset::~PreferencesReset()
{
	delete ui;
}

void PreferencesReset::on_resetSettings_clicked()
{
	auto dialog = PreferencesDialog::instance();
	dialog->defaultsRequested();
}

void PreferencesReset::refreshSettings()
{
}

void PreferencesReset::syncSettings()
{
}

