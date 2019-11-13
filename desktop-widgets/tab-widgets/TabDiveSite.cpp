// SPDX-License-Identifier: GPL-2.0
#include "TabDiveSite.h"
#include "core/subsurface-qt/DiveListNotifier.h"
#include "core/divesite.h"
#include "qt-models/divelocationmodel.h"
#include "qt-models/filtermodels.h"
#include "commands/command.h"

#include <qt-models/divecomputerextradatamodel.h>

TabDiveSite::TabDiveSite(QWidget *parent) : TabBase(parent)
{
	ui.setupUi(this);
	ui.diveSites->setTitle(tr("Dive sites"));
	ui.diveSites->setModel(&model);
	// Default: sort by name
	ui.diveSites->view()->sortByColumn(LocationInformationModel::NAME, Qt::AscendingOrder);
	ui.diveSites->view()->setSortingEnabled(true);
	ui.diveSites->view()->horizontalHeader()->setSectionResizeMode(LocationInformationModel::NAME, QHeaderView::Stretch);
	ui.diveSites->view()->horizontalHeader()->setSectionResizeMode(LocationInformationModel::DESCRIPTION, QHeaderView::Stretch);
	ui.diveSites->view()->setSelectionBehavior(QAbstractItemView::SelectRows);

	// Show only the first few columns
	for (int i = LocationInformationModel::LOCATION; i < LocationInformationModel::COLUMNS; ++i)
		ui.diveSites->view()->setColumnHidden(i, true);

	connect(ui.diveSites, &TableView::addButtonClicked, this, &TabDiveSite::add);
	connect(ui.diveSites->view()->selectionModel(), &QItemSelectionModel::selectionChanged, this, &TabDiveSite::selectionChanged);

	// Subtle: We depend on this slot being executed after the slot in the model.
	// This is realized because the model was constructed as a member object and connects in the constructor.
	connect(&diveListNotifier, &DiveListNotifier::diveSiteChanged, this, &TabDiveSite::diveSiteChanged);
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

void TabDiveSite::diveSiteChanged(struct dive_site *ds, int field)
{
	int idx = get_divesite_idx(ds, &dive_site_table);
	if (idx < 0)
		return;
	QModelIndex globalIdx = LocationInformationModel::instance()->index(idx, field);
	QModelIndex localIdx = model.mapFromSource(globalIdx);
	ui.diveSites->view()->scrollTo(localIdx);
}

void TabDiveSite::on_purgeUnused_clicked()
{
	Command::purgeUnusedDiveSites();
}

void TabDiveSite::on_filterText_textChanged(const QString &text)
{
	model.setFilter(text);
}

QVector<dive_site *> TabDiveSite::selectedDiveSites()
{
	const QModelIndexList indexes = ui.diveSites->view()->selectionModel()->selectedRows();
	QVector<dive_site *> sites;
	sites.reserve(indexes.size());
	for (const QModelIndex &idx: indexes) {
		struct dive_site *ds = model.getDiveSite(idx);
		sites.append(ds);
	}
	return sites;
}

void TabDiveSite::selectionChanged(const QItemSelection &, const QItemSelection &)
{
	MultiFilterSortModel::instance()->setFilterDiveSite(selectedDiveSites());
}

void TabDiveSite::showEvent(QShowEvent *)
{
	// If the user switches to the dive site tab and there was already a selection,
	// filter on that selection.
	MultiFilterSortModel::instance()->startFilterDiveSites(selectedDiveSites());
}

void TabDiveSite::hideEvent(QHideEvent *)
{
	// If the user switches to a different tab, stop the dive site filtering
	MultiFilterSortModel::instance()->stopFilterDiveSites();
}
