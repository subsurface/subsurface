// SPDX-License-Identifier: GPL-2.0
#include "preferences_dc.h"
#include "ui_preferences_dc.h"
#include "core/dive.h"
#include "core/settings/qPrefDisplay.h"
#include "core/settings/qPrefCloudStorage.h"
#include "core/settings/qPrefDiveComputer.h"

#include <QFileDialog>
#include <QProcess>
#include <QMessageBox>

PreferencesDc::PreferencesDc(): AbstractPreferencesWidget(tr("Dive download"), QIcon(":preferences-dc-icon"), 3 ), ui(new Ui::PreferencesDc())
{
	ui->setupUi(this);
	const QSize BUTTON_SIZE = QSize(200, 22);
	ui->resetRememberedDCs->resize(BUTTON_SIZE);
}

PreferencesDc::~PreferencesDc()
{
	delete ui;
}

void PreferencesDc::on_resetRememberedDCs_clicked()
{
	qPrefDiveComputer::set_vendor1(QString());
	qPrefDiveComputer::set_vendor2(QString());
	qPrefDiveComputer::set_vendor3(QString());
	qPrefDiveComputer::set_vendor4(QString());
}

void PreferencesDc::refreshSettings()
{
}

void PreferencesDc::syncSettings()
{
}
