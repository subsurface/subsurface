// SPDX-License-Identifier: GPL-2.0
#ifndef TANKINFOMODEL_H
#define TANKINFOMODEL_H

#include "cleanertablemodel.h"

/* Encapsulates the tank_info global variable
 * to show on Qt's Model View System. The list is always kept sorted
 * alphabetically by name (case-insensitive) - see add() and setData()'s
 * DESCRIPTION branch - so there is no manual reordering. */
class TankInfoModel : public CleanerTableModel {
	Q_OBJECT
public:
	enum Column {
		DESCRIPTION,
		ML,
		BAR
	};
	TankInfoModel(QObject *parent);

	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
	int rowCount(const QModelIndex &parent = QModelIndex()) const override;
	Qt::ItemFlags flags(const QModelIndex &index) const override;
	bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;

	// Management functions used by the cylinder-type editor dialog.
	// add() returns the row the new entry landed on, which - since the list
	// is kept sorted - isn't necessarily the last one.
	int add();
	void remove(int row);

signals:
	// Emitted after setData() applies a DESCRIPTION (rename) edit. Lets
	// callers that need to track a specific entry across the dialog's
	// lifetime (e.g. the "default cylinder" combo in Preferences) follow it
	// by identity through a rename, rather than relying on a name string
	// captured before the dialog ran, which would go stale as soon as that
	// exact entry is renamed.
	void cylinderTypeRenamed(const QString &oldName, const QString &newName);
};

#endif
