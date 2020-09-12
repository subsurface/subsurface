// SPDX-License-Identifier: GPL-2.0
#include "TabDiveComputer.h"
#include "ui_TabDiveExtraInfo.h"

TabDiveComputer::TabDiveComputer(QWidget *parent) : TabBase(parent)
{
	ui.setupUi(this);
	sortedModel.setSourceModel(&model);
	ui.table->setModel(&sortedModel);
	ui.table->view()->setSelectionBehavior(QAbstractItemView::SelectRows);
	ui.table->view()->setSelectionMode(QAbstractItemView::SingleSelection);
	ui.table->view()->setSortingEnabled(true);
	ui.table->view()->sortByColumn(DiveComputerModel::MODEL, Qt::AscendingOrder);
	connect(ui.table, &TableView::itemClicked, this, &TabDiveComputer::tableClicked);
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
		sortedModel.remove(index);
}
