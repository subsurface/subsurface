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

#include "ui_maintab.h"
#include "qt-models/completionmodels.h"
#include "qt-models/divelocationmodel.h"
#include "core/dive.h"
#include "core/subsurface-qt/divelistnotifier.h"

class ExtraDataModel;
class DivePictureModel;

class TabBase;
class MainTab : public QTabWidget {
	Q_OBJECT
public:
	MainTab(QWidget *parent = 0);
	void clearTabs();
	bool isEditing();
	void refreshDisplayedDiveSite();
	void nextInputField(QKeyEvent *event);
	void stealFocus();

public
slots:
	void divesChanged(const QVector<dive *> &dives, DiveField field);
	void diveSiteEdited(dive_site *ds, int field);
	void tripChanged(dive_trip *trip, TripField field);
	void updateDiveInfo();
	void updateNotes(const struct dive *d);
	void updateDateTime(const struct dive *d);
	void updateTripDate(const struct dive_trip *t);
	void updateDiveSite(struct dive *d);
	void acceptChanges();
	void rejectChanges();
	void on_location_diveSiteSelected();
	void on_locationPopupButton_clicked();
	void on_divemaster_editingFinished();
	void on_buddy_editingFinished();
	void on_diveTripLocation_editingFinished();
	void on_notes_editingFinished();
	void on_duration_editingFinished();
	void on_depth_editingFinished();
	void on_dateEdit_editingFinished();
	void on_timeEdit_editingFinished();
	void on_rating_valueChanged(int value);
	void on_tagWidget_editingFinished();
	void hideMessage();
	void closeMessage();
	void closeWarning();
	void displayMessage(QString str);
	void enableEdition();
	void escDetected(void);
	void updateDateTimeFields();
	void colorsChanged();
private:
	Ui::MainTab ui;
	bool editMode;
	bool ignoreInput; // When computionally editing fields, we have to ignore changed-signals
	BuddyCompletionModel buddyModel;
	DiveMasterCompletionModel diveMasterModel;
	TagCompletionModel tagModel;
	bool lastSelectedDive;
	int lastTabSelectedDive;
	int lastTabSelectedDiveTrip;
	dive_trip *currentTrip;
	QList<TabBase*> extraWidgets;
	void divesEdited(int num); // Opens a warning window if more than one dive was edited
	void changeEvent(QEvent *ev) override;
	bool isDark;
};

#endif // MAINTAB_H
