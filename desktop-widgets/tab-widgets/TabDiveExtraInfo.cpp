#include "TabDiveExtraInfo.h"
#include "ui_TabDiveExtraInfo.h"

#include <qt-models/divecomputerextradatamodel.h>

TabDiveExtraInfo::TabDiveExtraInfo(QWidget *parent) :
	TabBase(parent),
	ui(new Ui::TabDiveExtraInfo()),
	extraDataModel(new ExtraDataModel())
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
	extraDataModel->updateDive();
}

void TabDiveExtraInfo::clear()
{
	extraDataModel->updateDive();
}

