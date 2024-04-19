// SPDX-License-Identifier: GPL-2.0
#include "preferences_reset.h"
#include "ui_preferences_reset.h"
#include "preferencesdialog.h"

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
