/*
 * maintab.h
 *
 * header file for the main tab of Subsurface
 *
 */
#ifndef MAINTAB_H
#define MAINTAB_H

#include <QTabWidget>
#include <QDialog>

#include "models.h"

namespace Ui
{
	class MainTab;
}

struct NotesBackup{
	QString location;
	QString notes;
	QString buddy;
	QString suit;
	int rating;
	QString divemaster;
};

class MainTab : public QTabWidget
{
	Q_OBJECT
public:
	MainTab(QWidget *parent);
	void clearStats();
	void clearInfo();
	void clearEquipment();
	void reload();

public Q_SLOTS:
	void on_addCylinder_clicked();
	void on_editCylinder_clicked();
	void on_delCylinder_clicked();
	void on_addWeight_clicked();
	void on_editWeight_clicked();
	void on_delWeight_clicked();
	void updateDiveInfo(int dive);
	void on_editNotes_clicked(bool edit);
	void on_resetNotes_clicked();
	void on_location_textChanged(const QString& text);
	void on_divemaster_textChanged(const QString& text);
	void on_buddy_textChanged(const QString& text);
	void on_suit_textChanged(const QString& text);
	void on_notes_textChanged();
	void on_rating_valueChanged(int value);

private:
	Ui::MainTab *ui;
	WeightModel *weightModel;
	CylindersModel *cylindersModel;
	NotesBackup notesBackup;
	struct dive* currentDive;
};

#endif
