// SPDX-License-Identifier: GPL-2.0
#ifndef MANAGETANKINFODIALOG_H
#define MANAGETANKINFODIALOG_H

#include <QDialog>

namespace Ui {
	class ManageTankInfoDialog;
}

class TankInfoModel;
class QModelIndex;

/* Preferences-level editor for the global cylinder (tank) type list
 * (tank_info_table). Lets the user rename, resize and delete the entries
 * that show up in the per-dive cylinder-type selector; the list itself is
 * always shown sorted alphabetically by name (see TankInfoModel), so there
 * is no manual reordering. */
class ManageTankInfoDialog : public QDialog {
	Q_OBJECT
public:
	// trackedName is an optional cylinder-type name (e.g. the Preferences
	// page's current "default cylinder" selection) that the dialog follows
	// across renames made while it's open - see trackedName() below.
	explicit ManageTankInfoDialog(const QString &trackedName = QString(), QWidget *parent = nullptr);
	~ManageTankInfoDialog();

	// Persist the (already live-applied) edits regardless of how the dialog
	// is dismissed (Close button, Esc, or the window's close box all end up
	// calling done()), so they survive application restarts.
	void done(int result) override;

	// The name passed to the constructor, followed through any rename(s)
	// of that same entry made while the dialog was open (unchanged if it
	// was never renamed). Callers that captured a name before opening the
	// dialog should use this instead of their original string when looking
	// the entry back up afterwards, since a rename of that exact entry
	// makes the original string stale.
	QString trackedName() const;

private slots:
	void addType();
	void removeSelected();
	void updateButtons();
	void onCylinderTypeRenamed(const QString &oldName, const QString &newName);

private:
	Ui::ManageTankInfoDialog *ui;
	TankInfoModel *model;
	QString tracked;
};

#endif
