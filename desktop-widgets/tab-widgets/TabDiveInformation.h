// SPDX-License-Identifier: GPL-2.0
#ifndef TAB_DIVE_INFORMATION_H
#define TAB_DIVE_INFORMATION_H

#include "TabBase.h"
#include "core/subsurface-qt/divelistnotifier.h"

namespace Ui {
	class TabDiveInformation;
};

class TabDiveInformation : public TabBase {
	Q_OBJECT
public:
	TabDiveInformation(MainTab *parent);
	~TabDiveInformation();
	void updateData(const std::vector<dive *> &selection, dive *currentDive, int currentDC) override;
	void clear() override;
	void updateUi(QString titleColor) override;
private slots:
	void divesChanged(const QVector<dive *> &dives, DiveField field);
	void cylinderChanged(dive *d);
	void diveModeChanged(int index);
	void on_atmPressVal_editingFinished();
	void on_atmPressType_currentIndexChanged(int index);
	void on_visibility_valueChanged(int value);
	void on_wavesize_valueChanged(int value);
	void on_current_valueChanged(int value);
	void on_surge_valueChanged(int value);
	void on_chill_valueChanged(int value);
	void on_airtemp_editingFinished();
	void on_watertemp_editingFinished();
	void on_waterTypeCombo_activated(int index);
private:
	Ui::TabDiveInformation *ui;
	void updateProfile();
	int updateSalinityComboIndex(int salinity);
	void checkDcSalinityOverWritten();
	void updateWhen();
	int pressTypeIndex;
	void updateWaterTypeWidget();
	void updateTextBox(int event);
	void updateMode();
	void divesEdited(int);
	void closeWarning();
	void showCurrentWidget(bool show, int position);
};

#endif
