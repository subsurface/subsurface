// SPDX-License-Identifier: GPL-2.0
#ifndef FILTERPRESETMODEL_H
#define FILTERPRESETMODEL_H

#include "cleanertablemodel.h"
#include "core/filterpreset.h"

class FilterPresetModel : public CleanerTableModel {
	Q_OBJECT
public:
	// For QML we will have to define roles
	enum Column {
		REMOVE,
		NAME
	};
private
slots:
	void reset();
	void filterPresetAdded(int index);
	void filterPresetChanged(int index);
	void filterPresetRemoved(int index);
public:
	// there is one global filter preset list, therefore this model is a singleton
	static FilterPresetModel *instance();
private:
	FilterPresetModel();
	~FilterPresetModel();
	QVariant data(const QModelIndex &index, int role) const override;
	int rowCount(const QModelIndex &parent) const override;
	std::vector<filter_constraint> constraints;
};

#endif
