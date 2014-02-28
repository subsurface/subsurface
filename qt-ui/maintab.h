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
#include "ui_maintab.h"
#include "completionmodels.h"

class QCompleter;
struct dive;

struct NotesBackup {
	QString airtemp;
	QString watertemp;
	QString datetime;
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
	QString tags;
	cylinder_t cylinders[MAX_CYLINDERS];
	weightsystem_t weightsystem[MAX_WEIGHTSYSTEMS];
};

struct Completers {
	QCompleter *location;
	QCompleter *divemaster;
	QCompleter *buddy;
	QCompleter *suit;
	QCompleter *tags;
};

class MainTab : public QTabWidget {
	Q_OBJECT
public:
	enum EditMode {
		NONE,
		DIVE,
		TRIP,
		ADD,
		MANUALLY_ADDED_DIVE
	};

	MainTab(QWidget *parent);
	~MainTab();
	void clearStats();
	void clearInfo();
	void clearEquipment();
	void reload();
	bool eventFilter(QObject *, QEvent *);
	void initialUiSetup();
	bool isEditing();
	void updateCoordinatesText(qreal lat, qreal lon);
public
slots:
	void addCylinder_clicked();
	void addWeight_clicked();
	void updateDiveInfo(int dive = selected_dive);
	void acceptChanges();
	void rejectChanges();
	void on_location_textChanged(const QString &text);
	void on_coordinates_textChanged(const QString &text);
	void on_divemaster_textChanged();
	void on_buddy_textChanged();
	void on_suit_textChanged(const QString &text);
	void on_notes_textChanged();
	void on_airtemp_textChanged(const QString &text);
	void on_watertemp_textChanged(const QString &text);
	void on_dateTimeEdit_dateTimeChanged(const QDateTime &datetime);
	void on_rating_valueChanged(int value);
	void on_visibility_valueChanged(int value);
	void on_tagWidget_textChanged();
	void editCylinderWidget(const QModelIndex &index);
	void editWeightWidget(const QModelIndex &index);
	void addDiveStarted();
	void addMessageAction(QAction *action);
	void hideMessage();
	void closeMessage();
	void displayMessage(QString str);
	void enableEdition(EditMode newEditMode = NONE);
	void toggleTriggeredColumn();

private:
	Ui::MainTab ui;
	WeightModel *weightModel;
	CylindersModel *cylindersModel;
	QMap<dive *, NotesBackup> notesBackup;
	EditMode editMode;

	BuddyCompletionModel buddyModel;
	DiveMasterCompletionModel diveMasterModel;
	LocationCompletionModel locationModel;
	SuitCompletionModel suitModel;
	TagCompletionModel tagModel;

	/* since the multi-edition of the equipment is fairly more
	 * complex than a single item, because it involves a Qt
	 * Model to edit things, we are copying the first selected
	 * dive to this structure, making all editions there,
	 * then applying the changes on the other dives.*/
	struct dive multiEditEquipmentPlaceholder;
	Completers completers;
	void resetPallete();
	void saveTags();
	QString printGPSCoords(int lat, int lon);
	void updateGpsCoordinates(const struct dive *dive);
};

#endif // MAINTAB_H
