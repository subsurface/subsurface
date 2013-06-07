#include "divecomputermanagementdialog.h"
#include "models.h"
#include "ui_divecomputermanagementdialog.h"
#include "mainwindow.h"
#include <QMessageBox>

DiveComputerManagementDialog::DiveComputerManagementDialog(QWidget* parent, Qt::WindowFlags f): QDialog(parent, f)
, ui( new Ui::DiveComputerManagementDialog())
{
	ui->setupUi(this);
	model = new DiveComputerModel();
	ui->tableView->setModel(model);
	connect(ui->tableView, SIGNAL(clicked(QModelIndex)), this, SLOT(tryRemove(QModelIndex)));
	ui->tableView->setColumnWidth(DiveComputerModel::REMOVE, 22);
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

void DiveComputerManagementDialog::tryRemove(const QModelIndex& index)
{
	if (index.column() != DiveComputerModel::REMOVE){
		return;
	}
	
	QMessageBox::StandardButton response = QMessageBox::question(
		this, 
		tr("Remove the selected Dive Computer?"), 
		tr("Are you sure that you want to \n remove the selected dive computer?"),
		QMessageBox::Ok | QMessageBox::Cancel
	);
	
	if (response == QMessageBox::Ok){
		model->remove(index);
	}
}
