// SPDX-License-Identifier: GPL-2.0
#ifndef TAB_DIVE_SITE_H
#define TAB_DIVE_SITE_H

#include "TabBase.h"
#include "ui_TabDiveSite.h"
#include "qt-models/divelocationmodel.h"

class TabDiveSite : public TabBase {
	Q_OBJECT
public:
	TabDiveSite(QWidget *parent = 0);
	void updateData() override;
	void clear() override;
private:
	Ui::TabDiveSite ui;
	DiveSiteSortedModel model;
};

#endif
