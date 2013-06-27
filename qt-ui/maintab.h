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
	int visibility;
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

	bool eventFilter(QObject* , QEvent*);
	virtual void resizeEvent(QResizeEvent*);
	virtual void showEvent(QShowEvent*);
    virtual void hideEvent(QHideEvent* );

	void initialUiSetup();
	void equipmentPlusUpdate();


public slots:
	void addCylinder_clicked();
	void addWeight_clicked();
	void updateDiveInfo(int dive);
	void on_editAccept_clicked(bool edit);
	void on_editReset_clicked();
	void on_location_textChanged(const QString& text);
	void on_divemaster_textChanged(const QString& text);
	void on_buddy_textChanged(const QString& text);
	void on_suit_textChanged(const QString& text);
	void on_notes_textChanged();
	void on_rating_valueChanged(int value);
	void on_visibility_valueChanged(int value);
	void tabChanged(int idx);

private:
	Ui::MainTab *ui;
	WeightModel *weightModel;
	CylindersModel *cylindersModel;
	NotesBackup notesBackup;
	struct dive* currentDive;
	QPushButton *addCylinder;
	QPushButton *addWeight;
	enum { NONE, DIVE, TRIP } editMode;
};

#endif
