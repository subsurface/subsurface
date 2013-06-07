#include "divecomputermanagementdialog.h"
#include "models.h"
#include "ui_divecomputermanagementdialog.h"

DiveComputerManagementDialog::DiveComputerManagementDialog(QWidget* parent, Qt::WindowFlags f): QDialog(parent, f)
, ui( new Ui::DiveComputerManagementDialog())
{
	ui->setupUi(this);
	model = new DiveComputerModel();
	ui->tableView->setModel(model);
}

DiveComputerManagementDialog* DiveComputerManagementDialog::instance()
{
	static DiveComputerManagementDialog *self = new DiveComputerManagementDialog();
	return self;
}

void DiveComputerManagementDialog::update()
{
	model->update();
}
