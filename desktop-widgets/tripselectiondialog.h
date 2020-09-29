// SPDX-License-Identifier: GPL-2.0
#ifndef TRIPSELECTIONDIALOG_H
#define TRIPSELECTIONDIALOG_H

#include <QDialog>
#include "ui_tripselectiondialog.h"

struct dive_trip;

class TripSelectionDialog : public QDialog {
	Q_OBJECT
private
slots:
	void selectionChanged();
public:
	TripSelectionDialog(QWidget *parent); // Must pass in MainWindow for QShortcut hackery.
	dive_trip *getTrip(); // NULL if user canceled.
private:
	dive_trip *selectedTrip() const;
	Ui::TripSelectionDialog ui;
};

#endif // TRIPSELECTIONDIALOG_H
