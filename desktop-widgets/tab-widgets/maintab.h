// SPDX-License-Identifier: GPL-2.0
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
#include <QUuid>

#include "ui_maintab.h"
#include "qt-models/completionmodels.h"
#include "qt-models/divelocationmodel.h"
#include "core/dive.h"
#include "core/subsurface-qt/DiveListNotifier.h"

class WeightModel;
class CylindersModel;
class ExtraDataModel;
class DivePictureModel;
class QCompleter;

struct Completers {
	QCompleter *divemaster;
	QCompleter *buddy;
	QCompleter *suit;
	QCompleter *tags;
};

class TabBase;
class MainTab : public QTabWidget {
	Q_OBJECT
public:
	enum EditMode {
		NONE,
		DIVE,
		TRIP,
		ADD,
		MANUALLY_ADDED_DIVE,
		IGNORE
	};

	MainTab(QWidget *parent = 0);
	~MainTab();
	void clearTabs();
	void clearEquipment();
	void reload();
	void initialUiSetup();
	bool isEditing();
	void updateCoordinatesText(qreal lat, qreal lon);
	void refreshDisplayedDiveSite();
	void nextInputField(QKeyEvent *event);

signals:
	void addDiveFinished();
	void diveSiteChanged();
public
slots:
	void divesChanged(dive_trip *trip, const QVector<dive *> &dives, DiveField field);
	void addCylinder_clicked();
	void addWeight_clicked();
	void updateDiveInfo(bool clear = false);
	void updateNotes(const struct dive *d);
	void updateMode(struct dive *d);
	void updateDateTime(struct dive *d);
	void updateDiveSite(struct dive *d);
	void updateDepthDuration();
	void acceptChanges();
	void rejectChanges();
	void on_location_diveSiteSelected();
	void on_divemaster_editingFinished();
	void on_buddy_editingFinished();
	void on_suit_editingFinished();
	void on_diveTripLocation_textEdited(const QString& text);
	void on_notes_textChanged();
	void on_notes_editingFinished();
	void on_airtemp_editingFinished();
	void on_duration_editingFinished();
	void on_depth_editingFinished();
	void divetype_Changed(int);
	void on_watertemp_editingFinished();
	void on_dateEdit_dateChanged(const QDate &date);
	void on_timeEdit_timeChanged(const QTime & time);
	void on_rating_valueChanged(int value);
	void on_visibility_valueChanged(int value);
	void on_tagWidget_editingFinished();
	void editCylinderWidget(const QModelIndex &index);
	void editWeightWidget(const QModelIndex &index);
	void addDiveStarted();
	void addMessageAction(QAction *action);
	void hideMessage();
	void closeMessage();
	void displayMessage(QString str);
	void enableEdition(EditMode newEditMode = NONE);
	void toggleTriggeredColumn();
	void updateTextLabels(bool showUnits = true);
	void escDetected(void);
	EditMode getEditMode() const;
private:
	Ui::MainTab ui;
	WeightModel *weightModel;
	CylindersModel *cylindersModel;
	EditMode editMode;
	BuddyCompletionModel buddyModel;
	DiveMasterCompletionModel diveMasterModel;
	SuitCompletionModel suitModel;
	TagCompletionModel tagModel;
	Completers completers;
	bool modified;
	bool lastSelectedDive;
	int lastTabSelectedDive;
	int lastTabSelectedDiveTrip;
	void resetPallete();
	void copyTagsToDisplayedDive();
	void markChangedWidget(QWidget *w);
	dive_trip_t *currentTrip;
	dive_trip_t displayedTrip;
	QList<TabBase*> extraWidgets;
};

#endif // MAINTAB_H
