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

DivesiteImportDialog::DivesiteImportDialog(dive_site_table imported, QString source, QWidget *parent) : QDialog(parent),
	importedSites(std::move(imported)),
	importedSource(std::move(source)),
	divesiteImportedModel(std::make_unique<DivesiteImportedModel>(importedSites))
{
	QShortcut *close = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_W), this);
	QShortcut *quit = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_Q), this);

	int startingWidth = defaultModelFont().pointSize();

	ui.setupUi(this);
	ui.importedDivesitesView->setModel(divesiteImportedModel.get());
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

	connect(ui.importedDivesitesView, &QTableView::clicked, divesiteImportedModel.get(), &DivesiteImportedModel::changeSelected);
	connect(ui.selectAllButton, &QPushButton::clicked, divesiteImportedModel.get(), &DivesiteImportedModel::selectAll);
	connect(ui.unselectAllButton, &QPushButton::clicked, divesiteImportedModel.get(), &DivesiteImportedModel::selectNone);
	connect(close, SIGNAL(activated()), this, SLOT(close()));
	connect(quit, SIGNAL(activated()), parent, SLOT(close()));

	ui.ok->setEnabled(true);
}

DivesiteImportDialog::~DivesiteImportDialog()
{
}

void DivesiteImportDialog::on_cancel_clicked()
{
	done(-1);
}

void DivesiteImportDialog::on_ok_clicked()
{
	// delete non-selected dive sites
	dive_site_table selectedSites;
	for (size_t i = 0; i < importedSites.size(); i++)  {
		if (divesiteImportedModel->data(divesiteImportedModel->index(i, 0), Qt::CheckStateRole) == Qt::Checked)
			selectedSites.push_back(std::move(importedSites[i]));
	}
	importedSites.clear(); // Hopefully, the model is not used thereafter!

	Command::importDiveSites(std::move(selectedSites), importedSource);
	accept();
}
