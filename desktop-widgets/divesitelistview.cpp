// SPDX-License-Identifier: GPL-2.0
#include "divesitelistview.h"
#include "core/subsurface-qt/divelistnotifier.h"
#include "core/divelog.h"
#include "core/divesite.h"
#include "core/divefilter.h"
#include "qt-models/divelocationmodel.h"
#include "desktop-widgets/mainwindow.h"
#include "commands/command.h"

#include <QMessageBox>

DiveSiteListView::DiveSiteListView(QWidget *parent) : QWidget(parent)
{
	ui.setupUi(this);

	// What follows is duplicate code with locationinformation.cpp.
	// We might want to unify this.
	ui.diveSiteMessage->setCloseButtonVisible(false);

	QAction *acceptAction = new QAction(tr("Done"), this);
	connect(acceptAction, &QAction::triggered, this, &DiveSiteListView::done);

	ui.diveSiteMessage->setText(tr("Dive site management"));
	ui.diveSiteMessage->addAction(acceptAction);

	model = new DiveSiteSortedModel(this);
	ui.diveSites->setTitle(tr("Dive sites"));
	ui.diveSites->setModel(model);
	// Default: sort by name
	ui.diveSites->view()->sortByColumn(LocationInformationModel::NAME, Qt::AscendingOrder);
	ui.diveSites->view()->setSortingEnabled(true);
	ui.diveSites->view()->horizontalHeader()->setSectionResizeMode(LocationInformationModel::NAME, QHeaderView::Stretch);
	ui.diveSites->view()->horizontalHeader()->setSectionResizeMode(LocationInformationModel::DESCRIPTION, QHeaderView::Stretch);
	ui.diveSites->view()->setSelectionBehavior(QAbstractItemView::SelectRows);

	// Show only the first few columns
	for (int i = LocationInformationModel::LOCATION; i < LocationInformationModel::COLUMNS; ++i)
		ui.diveSites->view()->setColumnHidden(i, true);

	connect(ui.diveSites, &TableView::addButtonClicked, this, &DiveSiteListView::add);
	connect(ui.diveSites, &TableView::itemClicked, this, &DiveSiteListView::diveSiteClicked);
	connect(ui.diveSites->view()->selectionModel(), &QItemSelectionModel::selectionChanged, this, &DiveSiteListView::selectionChanged);

	// Subtle: We depend on this slot being executed after the slot in the model.
	// This is realized because the model was constructed as a member object and connects in the constructor.
	connect(&diveListNotifier, &DiveListNotifier::diveSiteChanged, this, &DiveSiteListView::diveSiteChanged);
}

void DiveSiteListView::done()
{
	MainWindow::instance()->enterPreviousState();
}

void DiveSiteListView::diveSiteClicked(const QModelIndex &index)
{
	struct dive_site *ds = model->getDiveSite(index);
	if (!ds)
		return;
	switch (index.column()) {
	case LocationInformationModel::EDIT:
		MainWindow::instance()->editDiveSite(ds);
		break;
	case LocationInformationModel::REMOVE:
		if (!ds->dives.empty() &&
		    QMessageBox::warning(this, tr("Delete dive site?"),
					 tr("This dive site has %n dive(s). Do you really want to delete it?\n", "", ds->dives.size()),
					 QMessageBox::Yes|QMessageBox::No) == QMessageBox::No)
				return;
		Command::deleteDiveSites(QVector<dive_site *>{ds});
		break;
	}
}

void DiveSiteListView::add()
{
	// This is mighty dirty: We hook into the "dive site added" signal and
	// select the name field of the added dive site when the command sends
	// the signal. This works only because we know that the model added the
	// connection first. Very subtle!
	// After the command has finished, the signal is disconnected so that dive
	// site names are not selected on regular redo / undo.
	connect(&diveListNotifier, &DiveListNotifier::diveSiteAdded, this, &DiveSiteListView::diveSiteAdded);
	Command::addDiveSite(tr("New dive site"));
	disconnect(&diveListNotifier, &DiveListNotifier::diveSiteAdded, this, &DiveSiteListView::diveSiteAdded);
}

void DiveSiteListView::diveSiteAdded(struct dive_site *, int idx)
{
	if (idx < 0)
		return;
	QModelIndex globalIdx = LocationInformationModel::instance()->index(idx, LocationInformationModel::NAME);
	QModelIndex localIdx = model->mapFromSource(globalIdx);
	ui.diveSites->view()->setCurrentIndex(localIdx);
	ui.diveSites->view()->edit(localIdx);
}

void DiveSiteListView::diveSiteChanged(struct dive_site *ds, int field)
{
	size_t idx = divelog.sites.get_idx(ds);
	if (idx == std::string::npos)
		return;
	QModelIndex globalIdx = LocationInformationModel::instance()->index(static_cast<int>(idx), field);
	QModelIndex localIdx = model->mapFromSource(globalIdx);
	ui.diveSites->view()->scrollTo(localIdx);
}

void DiveSiteListView::on_purgeUnused_clicked()
{
	Command::purgeUnusedDiveSites();
}

void DiveSiteListView::on_filterText_textChanged(const QString &text)
{
	model->setFilter(text);
}

std::vector<dive_site *> DiveSiteListView::selectedDiveSites()
{
	const QModelIndexList indices = ui.diveSites->view()->selectionModel()->selectedRows();
	std::vector<dive_site *> sites;
	sites.reserve(indices.size());
	for (const QModelIndex &idx: indices) {
		struct dive_site *ds = model->getDiveSite(idx);
		sites.push_back(ds);
	}
	return sites;
}

void DiveSiteListView::selectionChanged(const QItemSelection &, const QItemSelection &)
{
	DiveFilter::instance()->setFilterDiveSite(selectedDiveSites());
}

void DiveSiteListView::hideEvent(QHideEvent *)
{
	// If the user switches to the dive site tab and there was already a selection,
	// filter on that selection.
	DiveFilter::instance()->stopFilterDiveSites();
}

void DiveSiteListView::showEvent(QShowEvent *)
{
	// If the user switches to the dive site tab and there was already a selection,
	// filter on that selection.
	DiveFilter::instance()->startFilterDiveSites(selectedDiveSites());
}
