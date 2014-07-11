#include "divecomputermanagementdialog.h"
#include "models.h"
#include "mainwindow.h"
#include "qthelper.h"
#include "helpers.h"
#include <QMessageBox>
#include <QShortcut>

DiveComputerManagementDialog::DiveComputerManagementDialog(QWidget *parent, Qt::WindowFlags f) : QDialog(parent, f),
	model(0)
{
	ui.setupUi(this);
	init();
	connect(ui.tableView, SIGNAL(clicked(QModelIndex)), this, SLOT(tryRemove(QModelIndex)));
	QShortcut *close = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_W), this);
	connect(close, SIGNAL(activated()), this, SLOT(close()));
	QShortcut *quit = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_Q), this);
	connect(quit, SIGNAL(activated()), parent, SLOT(close()));
}

void DiveComputerManagementDialog::init()
{
	delete model;
	model = new DiveComputerModel(dcList.dcMap);
	ui.tableView->setModel(model);
}

DiveComputerManagementDialog *DiveComputerManagementDialog::instance()
{
	static DiveComputerManagementDialog *self = new DiveComputerManagementDialog(MainWindow::instance());
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

void DiveComputerManagementDialog::tryRemove(const QModelIndex &index)
{
	if (index.column() != DiveComputerModel::REMOVE)
		return;

	QMessageBox::StandardButton response = QMessageBox::question(
	    this, TITLE_OR_TEXT(
		      tr("Remove the selected dive computer?"),
		      tr("Are you sure that you want to \n remove the selected dive computer?")),
	    QMessageBox::Ok | QMessageBox::Cancel);

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
