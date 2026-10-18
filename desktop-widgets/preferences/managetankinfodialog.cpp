// SPDX-License-Identifier: GPL-2.0
#include "managetankinfodialog.h"
#include "ui_managetankinfodialog.h"
#include "qt-models/tankinfomodel.h"
#include "core/equipment.h"

#include <QAbstractItemView>
#include <QHeaderView>
#include <QItemSelectionModel>

ManageTankInfoDialog::ManageTankInfoDialog(const QString &trackedName, QWidget *parent) : QDialog(parent),
	ui(new Ui::ManageTankInfoDialog),
	model(new TankInfoModel(this)),
	tracked(trackedName)
{
	ui->setupUi(this);

	ui->tankTypes->setModel(model);
	ui->tankTypes->horizontalHeader()->setStretchLastSection(false);
	ui->tankTypes->horizontalHeader()->setSectionResizeMode(TankInfoModel::DESCRIPTION, QHeaderView::Stretch);
	ui->tankTypes->setSelectionBehavior(QAbstractItemView::SelectRows);
	ui->tankTypes->setSelectionMode(QAbstractItemView::SingleSelection);

	connect(ui->addButton, &QPushButton::clicked, this, &ManageTankInfoDialog::addType);
	connect(ui->removeButton, &QPushButton::clicked, this, &ManageTankInfoDialog::removeSelected);
	connect(ui->tankTypes->selectionModel(), &QItemSelectionModel::currentRowChanged,
		this, &ManageTankInfoDialog::updateButtons);
	connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::close);
	connect(model, &TankInfoModel::cylinderTypeRenamed, this, &ManageTankInfoDialog::onCylinderTypeRenamed);

	updateButtons();
}

ManageTankInfoDialog::~ManageTankInfoDialog()
{
	delete ui;
}

void ManageTankInfoDialog::done(int result)
{
	// Edits (rename / resize / delete) are already applied live to
	// tank_info_table; persist them now so they survive application
	// restarts and show up in every subsequently opened or created dive log.
	save_tank_info_table();
	QDialog::done(result);
}

QString ManageTankInfoDialog::trackedName() const
{
	return tracked;
}

void ManageTankInfoDialog::onCylinderTypeRenamed(const QString &oldName, const QString &newName)
{
	if (tracked == oldName)
		tracked = newName;
}

void ManageTankInfoDialog::addType()
{
	// The new entry doesn't necessarily land on the last row: the list is
	// always kept sorted alphabetically (see TankInfoModel::add()), so look
	// up where it actually ended up instead of assuming the end.
	int row = model->add();
	QModelIndex index = model->index(row, TankInfoModel::DESCRIPTION);
	ui->tankTypes->setCurrentIndex(index);
	ui->tankTypes->edit(index);
}

void ManageTankInfoDialog::removeSelected()
{
	QModelIndex index = ui->tankTypes->currentIndex();
	if (!index.isValid())
		return;
	model->remove(index.row());
}

void ManageTankInfoDialog::updateButtons()
{
	QModelIndex index = ui->tankTypes->currentIndex();
	ui->removeButton->setEnabled(index.isValid());
}
