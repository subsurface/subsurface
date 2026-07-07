// SPDX-License-Identifier: GPL-2.0
#ifndef TAB_DIVE_EXTRA_INFO_H
#define TAB_DIVE_EXTRA_INFO_H

#include "TabBase.h"

namespace Ui {
	class TabDiveExtraInfo;
};

class ExtraDataModel;
struct dive;

class TabDiveExtraInfo : public TabBase {
	Q_OBJECT
public:
	TabDiveExtraInfo(MainTab *parent);
	~TabDiveExtraInfo();
	void updateData(const std::vector<dive *> &selection, dive *currentDive, int currentDC) override;
	void clear() override;
signals:
	void dcChangeRequested(int dcIndex);
private slots:
	void onModelComboActivated(int index);
	void onSerialComboActivated(int index);
private:
	// Snapshot of every dive computer in currentDivePtr (built once per call).
	struct DCEntry {
		int     index;
		QString model;
		QString serial;
	};

	Ui::TabDiveExtraInfo *ui;
	ExtraDataModel *extraDataModel;
	dive *currentDivePtr = nullptr;
	QList<DCEntry> allDCs() const;                                  // single walk over number_of_computers()
	void populateSerialCombo(const QString &modelName, int selectedDCIdx);
	// Helpers for iterating dive computers by model name
	QStringList uniqueModelNames() const;
	int firstDCIndexForModel(const QString &model) const;
};

#endif
