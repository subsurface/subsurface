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
#include <QMap>

#include "models.h"

class QCompleter;
struct dive;
namespace Ui
{
	class MainTab;
}

struct NotesBackup{
	QString location;
	QString coordinates;
	degrees_t latitude;
	degrees_t longitude;
	QString notes;
	QString buddy;
	QString suit;
	int rating;
	int visibility;
	QString divemaster;
};

struct Completers{
	QCompleter *location;
	QCompleter *divemaster;
	QCompleter *buddy;
	QCompleter *suit;
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
	void initialUiSetup();
	void equipmentPlusUpdate();
public slots:
	void addCylinder_clicked();
	void addWeight_clicked();
	void updateDiveInfo(int dive);
	void acceptChanges();
	void rejectChanges();
	void on_location_textChanged(const QString& text);
	void on_coordinates_textChanged(const QString& text);
	void on_divemaster_textChanged(const QString& text);
	void on_buddy_textChanged(const QString& text);
	void on_suit_textChanged(const QString& text);
	void on_notes_textChanged();
	void on_rating_valueChanged(int value);
	void on_visibility_valueChanged(int value);
	void editCylinderWidget(const QModelIndex& index);
	void editWeigthWidget(const QModelIndex& index);

private:
	Ui::MainTab *ui;
	WeightModel *weightModel;
	CylindersModel *cylindersModel;
	QMap<dive*, NotesBackup> notesBackup;
	enum { NONE, DIVE, TRIP } editMode;
	Completers completers;
	void enableEdition();
};

#endif
