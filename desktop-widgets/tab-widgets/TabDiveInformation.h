// SPDX-License-Identifier: GPL-2.0
#ifndef TAB_DIVE_INFORMATION_H
#define TAB_DIVE_INFORMATION_H

#include "TabBase.h"

namespace Ui {
	class TabDiveInformation;
};

class TabDiveInformation : public TabBase {
	Q_OBJECT
public:
	TabDiveInformation(QWidget *parent = 0);
	~TabDiveInformation();
	void updateData();
	void clear();

private:
	Ui::TabDiveInformation *ui;
};


#endif
