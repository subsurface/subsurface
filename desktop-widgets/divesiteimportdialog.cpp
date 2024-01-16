// SPDX-License-Identifier: GPL-2.0
#include "desktop-widgets/divesiteimportdialog.h"
#include "commands/command.h"
#include "core/qthelper.h"
#include "core/metrics.h"
#include "core/subsurface-string.h"
#include "desktop-widgets/mainwindow.h"
#include "qt-models/divesiteimportmodel.h"
#include "qt-models/models.h"

#include <QShortcut>

// Caller keeps ownership of "imported". The contents of "imported" will be consumed on execution of the dialog.
// On return, it will be empty.
DivesiteImportDialog::DivesiteImportDialog(struct dive_site_table &imported, QString source, QWidget *parent) : QDialog(parent),
	importedSource(std::move(source))
{
	QShortcut *close = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_W), this);
	QShortcut *quit = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_Q), this);

	divesiteImportedModel = new DivesiteImportedModel(this);

	int startingWidth = defaultModelFont().pointSize();

	ui.setupUi(this);
	ui.importedDivesitesView->setModel(divesiteImportedModel);
	ui.importedDivesitesView->setSelectionBehavior(QAbstractItemView::SelectRows);
	ui.importedDivesitesView->setSelectionMode(QAbstractItemView::SingleSelection);
	ui.importedDivesitesView->horizontalHeader()->setStretchLastSection(true);
	ui.importedDivesitesView->verticalHeader()->setVisible(false);
	ui.importedDivesitesView->setColumnWidth(0, startingWidth * 14);
	ui.importedDivesitesView->setColumnWidth(1, startingWidth * 12);
	ui.importedDivesitesView->setColumnWidth(2, startingWidth * 8);
	ui.importedDivesitesView->setColumnWidth(3, startingWidth * 14);
	ui.selectAllButton->setEnabled(true);
	ui.unselectAllButton->setEnabled(true);

	connect(ui.importedDivesitesView, &QTableView::clicked, divesiteImportedModel, &DivesiteImportedModel::changeSelected);
	connect(ui.selectAllButton, &QPushButton::clicked, divesiteImportedModel, &DivesiteImportedModel::selectAll);
	connect(ui.unselectAllButton, &QPushButton::clicked, divesiteImportedModel, &DivesiteImportedModel::selectNone);
	connect(close, SIGNAL(activated()), this, SLOT(close()));
	connect(quit, SIGNAL(activated()), parent, SLOT(close()));

	ui.ok->setEnabled(true);

	importedSites = imported;
	imported.nr = imported.allocated = 0;
	imported.dive_sites = nullptr;

	divesiteImportedModel->repopulate(&importedSites);
}

DivesiteImportDialog::~DivesiteImportDialog()
{
	clear_dive_site_table(&importedSites);
}

void DivesiteImportDialog::on_cancel_clicked()
{
	clear_dive_site_table(&importedSites);
	done(-1);
}

void DivesiteImportDialog::on_ok_clicked()
{
	// delete non-selected dive sites
	struct dive_site_table selectedSites = empty_dive_site_table;
	for (int i = 0; i < importedSites.nr; i++)
		if (divesiteImportedModel->data(divesiteImportedModel->index(i, 0), Qt::CheckStateRole) == Qt::Checked) {
			struct dive_site *newSite = alloc_dive_site();
			copy_dive_site(importedSites.dive_sites[i], newSite);
			add_dive_site_to_table(newSite, &selectedSites);
		}

	Command::importDiveSites(&selectedSites, importedSource);
	clear_dive_site_table(&selectedSites);
	clear_dive_site_table(&importedSites);
	accept();
}
