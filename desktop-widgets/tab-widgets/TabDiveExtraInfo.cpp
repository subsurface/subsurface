// SPDX-License-Identifier: GPL-2.0
#include "TabDiveExtraInfo.h"
#include "ui_TabDiveExtraInfo.h"
#include "core/dive.h"
#include "core/selection.h"
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
	if (currentDive)
		extraDataModel->updateDiveComputer(currentDive->get_dc(currentDC));

	ui->extraData->setVisible(false); // This will cause the resize to include rows outside the current viewport
	ui->extraData->resizeColumnsToContents();
	ui->extraData->setVisible(true);
}

void TabDiveExtraInfo::clear()
{
	extraDataModel->clear();
}
