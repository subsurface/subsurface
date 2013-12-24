#include "divecomputermanagementdialog.h"
#include "models.h"
#include "mainwindow.h"
#include <QMessageBox>
#include "../qthelper.h"
#include "../helpers.h"

DiveComputerManagementDialog::DiveComputerManagementDialog(QWidget* parent, Qt::WindowFlags f): QDialog(parent, f),
	model(0)
{
	ui.setupUi(this);
	init();
	connect(ui.tableView, SIGNAL(clicked(QModelIndex)), this, SLOT(tryRemove(QModelIndex)));
}

void DiveComputerManagementDialog::init()
{
	delete model;
	model = new DiveComputerModel(dcList.dcMap);
	ui.tableView->setModel(model);
}

DiveComputerManagementDialog* DiveComputerManagementDialog::instance()
{
	static DiveComputerManagementDialog *self = new DiveComputerManagementDialog(mainWindow());
	self->setAttribute(Qt::WA_QuitOnClose, false);
	return self;
}

void DiveComputerManagementDialog::update()
{
	model->update();
	ui.tableView->resizeColumnsToContents();
	ui.tableView->setColumnWidth(DiveComputerModel::REMOVE, 22);
	layout()->activate();
}

void DiveComputerManagementDialog::tryRemove(const QModelIndex& index)
{
	if (index.column() != DiveComputerModel::REMOVE)
		return;

	QMessageBox::StandardButton response = QMessageBox::question(
		this, TITLE_OR_TEXT(
		tr("Remove the selected Dive Computer?"),
		tr("Are you sure that you want to \n remove the selected dive computer?")),
		QMessageBox::Ok | QMessageBox::Cancel
	);

	if (response == QMessageBox::Ok)
		model->remove(index);
}

void DiveComputerManagementDialog::accept()
{
	model->keepWorkingList();
	hide();
	close();
}

void DiveComputerManagementDialog::reject()
{
	model->dropWorkingList();
	hide();
	close();
}
