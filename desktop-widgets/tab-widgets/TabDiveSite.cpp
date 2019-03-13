// SPDX-License-Identifier: GPL-2.0
#include "TabDiveSite.h"
#include "core/subsurface-qt/DiveListNotifier.h"
#include "core/divesite.h"
#include "qt-models/divelocationmodel.h"
#include "desktop-widgets/command.h"

#include <qt-models/divecomputerextradatamodel.h>

TabDiveSite::TabDiveSite(QWidget *parent) : TabBase(parent)
{
	ui.setupUi(this);
	ui.diveSites->setTitle(tr("Dive sites"));
	ui.diveSites->setModel(&model);
	// Default: sort by name
	ui.diveSites->view()->sortByColumn(LocationInformationModel::NAME, Qt::AscendingOrder);
	ui.diveSites->view()->setSortingEnabled(true);

	// Show only the first few rows
	for (int i = LocationInformationModel::COORDS; i < LocationInformationModel::COLUMNS; ++i)
		ui.diveSites->view()->setColumnHidden(i, true);

	connect(ui.diveSites, &TableView::addButtonClicked, this, &TabDiveSite::add);
}

void TabDiveSite::updateData()
{
}

void TabDiveSite::clear()
{
}

void TabDiveSite::add()
{
	// This is mighty dirty: We hook into the "dive site added" signal and
	// select the name field of the added dive site when the command sends
	// the signal. This works only because we know that the model added the
	// connection first. Very subtle!
	// After the command has finished, the signal is disconnected so that dive
	// site names are not selected on regular redo / undo.
	connect(&diveListNotifier, &DiveListNotifier::diveSiteAdded, this, &TabDiveSite::diveSiteAdded);
	Command::addDiveSite(tr("New dive site"));
	disconnect(&diveListNotifier, &DiveListNotifier::diveSiteAdded, this, &TabDiveSite::diveSiteAdded);
}

void TabDiveSite::diveSiteAdded(struct dive_site *, int idx)
{
	if (idx < 0)
		return;
	QModelIndex globalIdx = LocationInformationModel::instance()->index(idx, LocationInformationModel::NAME);
	QModelIndex localIdx = model.mapFromSource(globalIdx);
	ui.diveSites->view()->setCurrentIndex(localIdx);
	ui.diveSites->view()->edit(localIdx);
}
