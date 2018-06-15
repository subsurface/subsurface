// SPDX-License-Identifier: GPL-2.0
#ifndef DIVECOMPUTERMANAGEMENTDIALOG_H
#define DIVECOMPUTERMANAGEMENTDIALOG_H
#include <QDialog>
#include "ui_divecomputermanagementdialog.h"
#include "qt-models/divecomputermodel.h"

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
	QScopedPointer<DiveComputerModel> model;
};

#endif // DIVECOMPUTERMANAGEMENTDIALOG_H
