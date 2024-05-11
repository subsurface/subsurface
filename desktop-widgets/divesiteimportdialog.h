// SPDX-License-Identifier: GPL-2.0
#ifndef DIVESITEIMPORTDIALOG_H
#define DIVESITEIMPORTDIALOG_H

#include <QDialog>
#include <QThread>
#include <QHash>
#include <QMap>
#include <QAbstractTableModel>
#include <memory>

#include "ui_divesiteimportdialog.h"
#include "core/divesite.h"

namespace Ui {
	class DivesiteImportDialog;
}

class DivesiteImportedModel;

class DivesiteImportDialog : public QDialog {
	Q_OBJECT
public:
	// Note: takes ownership of importedd table
	DivesiteImportDialog(dive_site_table imported, QString source, QWidget *parent = 0);
	~DivesiteImportDialog();

public
slots:
	void on_ok_clicked();
	void on_cancel_clicked();

private:
	Ui::DivesiteImportDialog ui;
	dive_site_table importedSites;
	QString importedSource;

	std::unique_ptr<DivesiteImportedModel> divesiteImportedModel;
};

#endif // DIVESITEIMPORTDIALOG_H
