// SPDX-License-Identifier: GPL-2.0
#ifndef DIVECOMPONENTSELECTION_H
#define DIVECOMPONENTSELECTION_H

#include "ui_divecomponentselection.h"

struct dive_paste_data;

class DiveComponentSelection : public QDialog {
	Q_OBJECT
public:
	explicit DiveComponentSelection(dive_paste_data &data, QWidget *parent = nullptr);
private
slots:
	void buttonClicked(QAbstractButton *button);

private:
	Ui::DiveComponentSelectionDialog ui;
	dive_paste_data &data;
};

#endif
