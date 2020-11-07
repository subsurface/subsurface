// SPDX-License-Identifier: GPL-2.0
#include "TabDiveComputer.h"
#include "ui_TabDiveExtraInfo.h"
#include "qt-models/divecomputermodel.h"

TabDiveComputer::TabDiveComputer(QWidget *parent) : TabBase(parent)
{
	ui.setupUi(this);

	DiveComputerModel *model = new DiveComputerModel(this);
	sortedModel = new DiveComputerSortedModel(this);

	sortedModel->setSourceModel(model);
	ui.devices->setModel(sortedModel);
	ui.devices->view()->setSelectionBehavior(QAbstractItemView::SelectRows);
	ui.devices->view()->setSelectionMode(QAbstractItemView::SingleSelection);
	ui.devices->view()->setSortingEnabled(true);
	ui.devices->view()->sortByColumn(DiveComputerModel::MODEL, Qt::AscendingOrder);
	ui.devices->view()->horizontalHeader()->setStretchLastSection(true);
	connect(ui.devices, &TableView::itemClicked, this, &TabDiveComputer::tableClicked);
}

void TabDiveComputer::updateData()
{
}

void TabDiveComputer::clear()
{
}

void TabDiveComputer::tableClicked(const QModelIndex &index)
{
	if (!index.isValid())
		return;

	if (index.column() == DiveComputerModel::REMOVE)
		sortedModel->remove(index);
}
