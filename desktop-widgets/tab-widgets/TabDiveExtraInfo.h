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
	TabDiveExtraInfo(QWidget *parent);
	~TabDiveExtraInfo();
	void updateData() override;
	void clear() override;
private:
	Ui::TabDiveExtraInfo *ui;
	ExtraDataModel *extraDataModel;
};

#endif
