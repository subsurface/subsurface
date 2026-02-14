// SPDX-License-Identifier: GPL-2.0
#include "TabDiveExtraInfo.h"
#include "ui_TabDiveExtraInfo.h"
#include "core/dive.h"
#include "core/gettextfromc.h"
#include "core/selection.h"
#include "core/string-format.h"
#include "qt-models/divecomputerextradatamodel.h"

TabDiveExtraInfo::TabDiveExtraInfo(MainTab *parent) :
	TabBase(parent),
	ui(new Ui::TabDiveExtraInfo()),
	extraDataModel(new ExtraDataModel(this))
{
	ui->setupUi(this);
	ui->extraData->setModel(extraDataModel);

	ui->extraData->horizontalHeader()->setStretchLastSection(true);
}

TabDiveExtraInfo::~TabDiveExtraInfo()
{
	delete ui;
}

void TabDiveExtraInfo::updateData(const std::vector<dive *> &, dive *currentDive, int currentDC)
{
	if (currentDive) {
		const divecomputer *dc = currentDive->get_dc(currentDC);

		ui->model->setText(QString::fromStdString(dc->model).trimmed());
		ui->serial->setText(QString::fromStdString(dc->serial).trimmed());
		ui->firmware->setText(QString::fromStdString(dc->fw_version).trimmed());
		ui->date->setText(get_dive_date_string(dc->when));
		ui->duration->setText(get_dive_duration_string(dc->duration.seconds, tr("h"), tr("min"), tr("sec"),
				" ", dc->divemode == FREEDIVE));
		if (dc->divemode >= 0 && dc->divemode < NUM_DIVEMODE)
			ui->diveMode->setText(gettextFromC::tr(divemode_text_ui[dc->divemode]));
		else
			ui->diveMode->clear();

		extraDataModel->updateDiveComputer(dc);
		ui->extraData->setVisible(false); // This will cause the resize to include rows outside the current viewport
		ui->extraData->resizeColumnsToContents();
		ui->extraData->setVisible(true);
	} else {
		clear();
	}
}

void TabDiveExtraInfo::clear()
{
	ui->model->clear();
	ui->serial->clear();
	ui->firmware->clear();
	ui->date->clear();
	ui->duration->clear();
	ui->diveMode->clear();
	extraDataModel->clear();
}
