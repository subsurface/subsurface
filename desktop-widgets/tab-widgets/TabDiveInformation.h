// SPDX-License-Identifier: GPL-2.0
#ifndef TAB_DIVE_INFORMATION_H
#define TAB_DIVE_INFORMATION_H

#include "TabBase.h"
#include "core/subsurface-qt/DiveListNotifier.h"

namespace Ui {
	class TabDiveInformation;
};

class TabDiveInformation : public TabBase {
	Q_OBJECT
public:
	TabDiveInformation(QWidget *parent = 0);
	~TabDiveInformation();
	void updateData() override;
	void clear() override;
private slots:
	void divesChanged(const QVector<dive *> &dives, DiveField field);
	void on_atmPressVal_editingFinished();
	void on_atmPressType_currentIndexChanged(int index);
private:
	Ui::TabDiveInformation *ui;
	void updateProfile();
	void updateSalinity();
	void updateWhen();
	int pressTypeIndex;
	void updateTextBox(int event);
};

#endif
