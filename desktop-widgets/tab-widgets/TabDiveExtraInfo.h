// SPDX-License-Identifier: GPL-2.0
#ifndef TAB_DIVE_EXTRA_INFO_H
#define TAB_DIVE_EXTRA_INFO_H

#include "TabBase.h"

namespace Ui {
	class TabDiveExtraInfo;
};

class ExtraDataModel;

class TabDiveExtraInfo : public TabBase {
	Q_OBJECT
public:
	TabDiveExtraInfo(QWidget *parent = 0);
	~TabDiveExtraInfo();
	void updateData();
	void clear();
private:
	Ui::TabDiveExtraInfo *ui;
	ExtraDataModel *extraDataModel;
};

#endif
