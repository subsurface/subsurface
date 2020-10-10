// SPDX-License-Identifier: GPL-2.0
#ifndef DIVECOMPUTERMANAGEMENTDIALOG_H
#define DIVECOMPUTERMANAGEMENTDIALOG_H

#include "ui_divecomputermanagementdialog.h"
#include "qt-models/divecomputermodel.h"
#include <QDialog>
#include <memory>

class QModelIndex;

class DiveComputerManagementDialog : public QDialog {
	Q_OBJECT

public:
	static DiveComputerManagementDialog *instance();
	void init();

public
slots:
	void tryRemove(const QModelIndex &index);
	void accept();
	void reject();

private:
	explicit DiveComputerManagementDialog(QWidget *parent = 0, Qt::WindowFlags f = 0);
	Ui::DiveComputerManagementDialog ui;
	std::unique_ptr<DiveComputerModel> model;
	DiveComputerSortedModel proxyModel;
};

#endif // DIVECOMPUTERMANAGEMENTDIALOG_H
