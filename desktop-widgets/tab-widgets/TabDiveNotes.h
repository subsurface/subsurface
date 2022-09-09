// SPDX-License-Identifier: GPL-2.0
// The notes tab.
#ifndef NOTESTAB_H
#define NOTESTAB_H

#include "TabBase.h"
#include "ui_TabDiveNotes.h"
#include "qt-models/completionmodels.h"
#include "qt-models/divelocationmodel.h"
#include "core/dive.h"
#include "core/subsurface-qt/divelistnotifier.h"

class ExtraDataModel;
class DivePictureModel;

class TabDiveNotes : public TabBase {
	Q_OBJECT
public:
	TabDiveNotes(MainTab *parent);
	void updateData(const std::vector<dive *> &selection, dive *currentDive, int currentDC) override;
	void clear() override;
	void closeWarning();

public
slots:
	void divesChanged(const QVector<dive *> &dives, DiveField field);
	void diveSiteEdited(dive_site *ds, int field);
	void tripChanged(dive_trip *trip, TripField field);
	void updateNotes(const struct dive *d);
	void updateDateTime(const struct dive *d);
	void updateTripDate(const struct dive_trip *t);
	void updateDiveSite(struct dive *d);
	void on_location_diveSiteSelected();
	void on_locationPopupButton_clicked();
	void on_diveguide_editingFinished();
	void on_buddy_editingFinished();
	void on_diveTripLocation_editingFinished();
	void on_notes_editingFinished();
	void on_duration_editingFinished();
	void on_depth_editingFinished();
	void on_dateEdit_editingFinished();
	void on_timeEdit_editingFinished();
	void on_rating_valueChanged(int value);
	void on_tagWidget_editingFinished();
	void updateDateTimeFields();
private:
	Ui::TabDiveNotes ui;
	bool ignoreInput; // When computionally editing fields, we have to ignore changed-signals
	dive_trip *currentTrip;
	BuddyCompletionModel buddyModel;
	DiveGuideCompletionModel diveGuideModel;
	TagCompletionModel tagModel;
	void divesEdited(int num); // Opens a warning window if more than one dive was edited
};

#endif // TABDIVENOTES_H
