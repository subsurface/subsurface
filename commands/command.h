// SPDX-License-Identifier: GPL-2.0
#ifndef COMMAND_H
#define COMMAND_H

#include "core/dive.h"
#include "core/pictureobj.h"
#include "core/taxonomy.h"
#include <QVector>
#include <QAction>
#include <vector>

struct DiveAndLocation;
struct FilterData;
struct filter_preset_table;
struct device_table;

// We put everything in a namespace, so that we can shorten names without polluting the global namespace
namespace Command {

// 1) General commands

void init();				// Setup signals to inform frontend of clean status.
void clear();				// Reset the undo stack. Delete all commands.
void setClean();			// Call after save - this marks a state where no changes need to be saved.
bool isClean();				// Any changes need to be saved?
QAction *undoAction(QObject *parent);	// Create an undo action.
QAction *redoAction(QObject *parent);	// Create an redo action.
QString changesMade();			// return a string with the texts from all commands on the undo stack -> for commit message

// 2) Dive-list related commands

// If d->dive_trip is null and autogroup is true, dives within the auto-group
// distance are added to a trip. dive d is consumed (the structure is reset)!
// If newNumber is true, the dive is assigned a new number, depending on the
// insertion position.
void addDive(dive *d, bool autogroup, bool newNumber);
void importDives(struct dive_table *dives, struct trip_table *trips,
		 struct dive_site_table *sites, struct device_table *devices,
		 struct filter_preset_table *filter_presets,
		 int flags, const QString &source); // The tables are consumed!
void deleteDive(const QVector<struct dive*> &divesToDelete);
void shiftTime(const std::vector<dive *> &changedDives, int amount);
void renumberDives(const QVector<QPair<dive *, int>> &divesToRenumber);
void removeDivesFromTrip(const QVector<dive *> &divesToRemove);
void removeAutogenTrips();
void addDivesToTrip(const QVector<dive *> &divesToAddIn, dive_trip *trip);
void createTrip(const QVector<dive *> &divesToAddIn);
void autogroupDives();
void mergeTrips(dive_trip *trip1, dive_trip *trip2);
void splitDives(dive *d, duration_t time);
void splitDiveComputer(dive *d, int dc_num);
void moveDiveComputerToFront(dive *d, int dc_num);
void deleteDiveComputer(dive *d, int dc_num);
void mergeDives(const QVector <dive *> &dives);
void applyGPSFixes(const std::vector<DiveAndLocation> &fixes);

// 3) Dive-site related commands

void deleteDiveSites(const QVector <dive_site *> &sites);
void editDiveSiteName(dive_site *ds, const QString &value);
void editDiveSiteDescription(dive_site *ds, const QString &value);
void editDiveSiteNotes(dive_site *ds, const QString &value);
void editDiveSiteCountry(dive_site *ds, const QString &value);
void editDiveSiteLocation(dive_site *ds, location_t value);
void editDiveSiteTaxonomy(dive_site *ds, taxonomy_data &value); // value is consumed (i.e. will be erased after call)!
void addDiveSite(const QString &name);
void importDiveSites(struct dive_site_table *sites, const QString &source);
void mergeDiveSites(dive_site *ds, const QVector<dive_site *> &sites);
void purgeUnusedDiveSites();

// 4) Dive editing related commands

int editNotes(const QString &newValue, bool currentDiveOnly);
int editSuit(const QString &newValue, bool currentDiveOnly);
int editMode(int index, int newValue, bool currentDiveOnly);
int editInvalid(int newValue, bool currentDiveOnly);
int editRating(int newValue, bool currentDiveOnly);
int editVisibility(int newValue, bool currentDiveOnly);
int editWaveSize(int newValue, bool currentDiveOnly);
int editCurrent(int newValue, bool currentDiveOnly);
int editSurge(int newValue, bool currentDiveOnly);
int editChill(int newValue, bool currentDiveOnly);
int editAirTemp(int newValue, bool currentDiveOnly);
int editWaterTemp(int newValue, bool currentDiveOnly);
int editAtmPress(int newValue, bool currentDiveOnly);
int editWaterTypeUser(int newValue, bool currentDiveOnly);
int editDepth(int newValue, bool currentDiveOnly);
int editDuration(int newValue, bool currentDiveOnly);
int editDiveSite(struct dive_site *newValue, bool currentDiveOnly);
int editDiveSiteNew(const QString &newName, bool currentDiveOnly);
int editTags(const QStringList &newList, bool currentDiveOnly);
int editBuddies(const QStringList &newList, bool currentDiveOnly);
int editDiveMaster(const QStringList &newList, bool currentDiveOnly);
void pasteDives(const dive *d, dive_components what);
void replanDive(dive *d); // dive computer(s) and cylinder(s) will be reset!
void editProfile(dive *d); // dive computer(s) and cylinder(s) will be reset!
int addWeight(bool currentDiveOnly);
int removeWeight(int index, bool currentDiveOnly);
int editWeight(int index, weightsystem_t ws, bool currentDiveOnly);
int addCylinder(bool currentDiveOnly);
int removeCylinder(int index, bool currentDiveOnly);
enum class EditCylinderType {
	TYPE,
	PRESSURE,
	GASMIX
};
int editCylinder(int index, cylinder_t cyl, EditCylinderType type, bool currentDiveOnly);
#ifdef SUBSURFACE_MOBILE
// Edits a dive and creates a divesite (if createDs != NULL) or edits a divesite (if changeDs != NULL).
// Takes ownership of newDive and createDs!
void editDive(dive *oldDive, dive *newDive, dive_site *createDs, dive_site *changeDs, location_t dsLocation);
#endif

// 5) Trip editing commands

void editTripLocation(dive_trip *trip, const QString &s);
void editTripNotes(dive_trip *trip, const QString &s);

// 6) Event commands

void addEventBookmark(struct dive *d, int dcNr, int seconds);
void addEventDivemodeSwitch(struct dive *d, int dcNr, int seconds, int divemode);
void addEventSetpointChange(struct dive *d, int dcNr, int seconds, pressure_t pO2);
void renameEvent(struct dive *d, int dcNr, struct event *ev, const char *name);
void removeEvent(struct dive *d, int dcNr, struct event *ev);
void addGasSwitch(struct dive *d, int dcNr, int seconds, int tank);

// 7) Picture (media) commands

struct PictureListForDeletion {
	dive *d;
	std::vector<std::string> filenames;
};
struct PictureListForAddition {
	dive *d;
	std::vector<PictureObj> pics;
};
void setPictureOffset(dive *d, const QString &filename, offset_t offset);
void removePictures(const std::vector<PictureListForDeletion> &pictures);
void addPictures(const std::vector<PictureListForAddition> &pictures);

// 8) Device commands

void removeDevice(int idx);
void editDeviceNickname(int idx, const QString &nickname);

// 9) Filter commands

void createFilterPreset(const QString &name, const FilterData &data);
void removeFilterPreset(int index);
void editFilterPreset(int index, const FilterData &data);

} // namespace Command

#endif // COMMAND_H
