// SPDX-License-Identifier: GPL-2.0
#include "TabDiveExtraInfo.h"
#include "ui_TabDiveExtraInfo.h"
#include "core/dive.h"
#include "core/selection.h"
#include "qt-models/divecomputerextradatamodel.h"

TabDiveExtraInfo::TabDiveExtraInfo(QWidget *parent) :
	TabBase(parent),
	ui(new Ui::TabDiveExtraInfo()),
	extraDataModel(new ExtraDataModel(this))
{
	ui->setupUi(this);
	ui->extraData->setModel(extraDataModel);
}

TabDiveExtraInfo::~TabDiveExtraInfo()
{
	delete ui;
}

void TabDiveExtraInfo::updateData()
{
	const struct divecomputer *currentdc = get_dive_dc(current_dive, dc_number);
	if (currentdc)
		extraDataModel->updateDiveComputer(currentdc);
}

void TabDiveExtraInfo::clear()
{
	extraDataModel->clear();
}
