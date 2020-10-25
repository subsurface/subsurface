// SPDX-License-Identifier: GPL-2.0

#include "command.h"
#include "command_device.h"
#include "command_divelist.h"
#include "command_divesite.h"
#include "command_edit.h"
#include "command_edit_trip.h"
#include "command_event.h"
#include "command_filter.h"
#include "command_pictures.h"

namespace Command {

// Dive-list related commands
void addDive(dive *d, bool autogroup, bool newNumber)
{
	execute(new AddDive(d, autogroup, newNumber));
}

void importDives(struct dive_table *dives, struct trip_table *trips, struct dive_site_table *sites,
		 struct device_table *devices, struct filter_preset_table *presets,
		 int flags, const QString &source)
{
	execute(new ImportDives(dives, trips, sites, devices, presets, flags, source));
}

void deleteDive(const QVector<struct dive*> &divesToDelete)
{
	execute(new DeleteDive(divesToDelete));
}

void shiftTime(const std::vector<dive *> &changedDives, int amount)
{
	execute(new ShiftTime(changedDives, amount));
}

void renumberDives(const QVector<QPair<dive *, int>> &divesToRenumber)
{
	execute(new RenumberDives(divesToRenumber));
}

void removeDivesFromTrip(const QVector<dive *> &divesToRemove)
{
	execute(new RemoveDivesFromTrip(divesToRemove));
}

void removeAutogenTrips()
{
	execute(new RemoveAutogenTrips);
}

void addDivesToTrip(const QVector<dive *> &divesToAddIn, dive_trip *trip)
{
	execute(new AddDivesToTrip(divesToAddIn, trip));
}

void createTrip(const QVector<dive *> &divesToAddIn)
{
	execute(new CreateTrip(divesToAddIn));
}

void autogroupDives()
{
	execute(new AutogroupDives);
}

void mergeTrips(dive_trip *trip1, dive_trip *trip2)
{
	execute(new MergeTrips(trip1, trip2));
}

void splitDives(dive *d, duration_t time)
{
	execute(new SplitDives(d, time));
}

void splitDiveComputer(dive *d, int dc_num)
{
	execute(new SplitDiveComputer(d, dc_num));
}

void moveDiveComputerToFront(dive *d, int dc_num)
{
	execute(new MoveDiveComputerToFront(d, dc_num));
}

void deleteDiveComputer(dive *d, int dc_num)
{
	execute(new DeleteDiveComputer(d, dc_num));
}

void mergeDives(const QVector <dive *> &dives)
{
	execute(new MergeDives(dives));
}

// Dive site related commands
void deleteDiveSites(const QVector <dive_site *> &sites)
{
	execute(new DeleteDiveSites(sites));
}

void editDiveSiteName(dive_site *ds, const QString &value)
{
	execute(new EditDiveSiteName(ds, value));
}

void editDiveSiteDescription(dive_site *ds, const QString &value)
{
	execute(new EditDiveSiteDescription(ds, value));
}

void editDiveSiteNotes(dive_site *ds, const QString &value)
{
	execute(new EditDiveSiteNotes(ds, value));
}

void editDiveSiteCountry(dive_site *ds, const QString &value)
{
	execute(new EditDiveSiteCountry(ds, value));
}

void editDiveSiteLocation(dive_site *ds, location_t value)
{
	execute(new EditDiveSiteLocation(ds, value));
}

void editDiveSiteTaxonomy(dive_site *ds, taxonomy_data &value)
{
	execute(new EditDiveSiteTaxonomy(ds, value));
}

void addDiveSite(const QString &name)
{
	execute(new AddDiveSite(name));
}

void importDiveSites(struct dive_site_table *sites, const QString &source)
{
	execute(new ImportDiveSites(sites, source));
}

void mergeDiveSites(dive_site *ds, const QVector<dive_site *> &sites)
{
	execute(new MergeDiveSites(ds, sites));
}

void purgeUnusedDiveSites()
{
	execute(new PurgeUnusedDiveSites);
}

void applyGPSFixes(const std::vector<DiveAndLocation> &fixes)
{
	execute(new ApplyGPSFixes(fixes));
}

// Execute an edit-command and return number of edited dives
static int execute_edit(EditDivesBase *cmd)
{
	int count = cmd->numDives();
	return execute(cmd) ? count : 0;
}

// Dive editing related commands
int editNotes(const QString &newValue, bool currentDiveOnly)
{
	return execute_edit(new EditNotes(newValue, currentDiveOnly));
}

int editMode(int index, int newValue, bool currentDiveOnly)
{
	return execute_edit(new EditMode(index, newValue, currentDiveOnly));
}

int editInvalid(int newValue, bool currentDiveOnly)
{
	return execute_edit(new EditInvalid(newValue, currentDiveOnly));
}

int editSuit(const QString &newValue, bool currentDiveOnly)
{
	return execute_edit(new EditSuit(newValue, currentDiveOnly));
}

int editRating(int newValue, bool currentDiveOnly)
{
	return execute_edit(new EditRating(newValue, currentDiveOnly));
}

int editVisibility(int newValue, bool currentDiveOnly)
{
	return execute_edit(new EditVisibility(newValue, currentDiveOnly));
}

int editWaveSize(int newValue, bool currentDiveOnly)
{
	return execute_edit(new EditWaveSize(newValue, currentDiveOnly));
}

int editCurrent(int newValue, bool currentDiveOnly)
{
	return execute_edit(new EditCurrent(newValue, currentDiveOnly));
}

int editSurge(int newValue, bool currentDiveOnly)
{
	return execute_edit(new EditSurge(newValue, currentDiveOnly));
}

int editChill(int newValue, bool currentDiveOnly)
{
	return execute_edit(new EditChill(newValue, currentDiveOnly));
}

int editAirTemp(int newValue, bool currentDiveOnly)
{
	return execute_edit(new EditAirTemp(newValue, currentDiveOnly));
}

int editWaterTemp(int newValue, bool currentDiveOnly)
{
	return execute_edit(new EditWaterTemp(newValue, currentDiveOnly));
}

int editAtmPress(int newValue, bool currentDiveOnly)
{
	return execute_edit(new EditAtmPress(newValue, currentDiveOnly));
}

int editWaterTypeUser(int newValue, bool currentDiveOnly)
{
	return execute_edit(new EditWaterTypeUser(newValue, currentDiveOnly));
}

int editDepth(int newValue, bool currentDiveOnly)
{
	return execute_edit(new EditDepth(newValue, currentDiveOnly));
}

int editDuration(int newValue, bool currentDiveOnly)
{
	return execute_edit(new EditDuration(newValue, currentDiveOnly));
}

int editDiveSite(struct dive_site *newValue, bool currentDiveOnly)
{
	return execute_edit(new EditDiveSite(newValue, currentDiveOnly));
}

int editDiveSiteNew(const QString &newName, bool currentDiveOnly)
{
	return execute_edit(new EditDiveSiteNew(newName, currentDiveOnly));
}

int editTags(const QStringList &newList, bool currentDiveOnly)
{
	return execute_edit(new EditTags(newList, currentDiveOnly));
}

int editBuddies(const QStringList &newList, bool currentDiveOnly)
{
	return execute_edit(new EditBuddies(newList, currentDiveOnly));
}

int editDiveMaster(const QStringList &newList, bool currentDiveOnly)
{
	return execute_edit(new EditDiveMaster(newList, currentDiveOnly));
}

void pasteDives(const dive *d, dive_components what)
{
	execute(new PasteDives(d, what));
}

void replanDive(dive *d)
{
	execute(new ReplanDive(d, false));
}

void editProfile(dive *d)
{
	execute(new ReplanDive(d, true));
}

int addWeight(bool currentDiveOnly)
{
	return execute_edit(new AddWeight(currentDiveOnly));
}

int removeWeight(int index, bool currentDiveOnly)
{
	return execute_edit(new RemoveWeight(index, currentDiveOnly));
}

int editWeight(int index, weightsystem_t ws, bool currentDiveOnly)
{
	return execute_edit(new EditWeight(index, ws, currentDiveOnly));
}

int addCylinder(bool currentDiveOnly)
{
	return execute_edit(new AddCylinder(currentDiveOnly));
}

int removeCylinder(int index, bool currentDiveOnly)
{
	return execute_edit(new RemoveCylinder(index, currentDiveOnly));
}

int editCylinder(int index, cylinder_t cyl, EditCylinderType type, bool currentDiveOnly)
{
	return execute_edit(new EditCylinder(index, cyl, type, currentDiveOnly));
}

// Trip editing related commands
void editTripLocation(dive_trip *trip, const QString &s)
{
	execute(new EditTripLocation(trip, s));
}

void editTripNotes(dive_trip *trip, const QString &s)
{
	execute(new EditTripNotes(trip, s));
}

#ifdef SUBSURFACE_MOBILE
void editDive(dive *oldDive, dive *newDive, dive_site *createDs, dive_site *changeDs, location_t dsLocation)
{
	execute(new EditDive(oldDive, newDive, createDs, changeDs, dsLocation));
}
#endif // SUBSURFACE_MOBILE

// Event commands

void addEventBookmark(struct dive *d, int dcNr, int seconds)
{
	execute(new AddEventBookmark(d, dcNr, seconds));
}

void addEventDivemodeSwitch(struct dive *d, int dcNr, int seconds, int divemode)
{
	execute(new AddEventDivemodeSwitch(d, dcNr, seconds, divemode));
}

void addEventSetpointChange(struct dive *d, int dcNr, int seconds, pressure_t pO2)
{
	execute(new AddEventSetpointChange(d, dcNr, seconds, pO2));
}

void renameEvent(struct dive *d, int dcNr, struct event *ev, const char *name)
{
	execute(new RenameEvent(d, dcNr, ev, name));
}

void removeEvent(struct dive *d, int dcNr, struct event *ev)
{
	execute(new RemoveEvent(d, dcNr, ev));
}

void addGasSwitch(struct dive *d, int dcNr, int seconds, int tank)
{
	execute(new AddGasSwitch(d, dcNr, seconds, tank));
}

// Picture (media) commands

void setPictureOffset(dive *d, const QString &filename, offset_t offset)
{
	execute(new SetPictureOffset(d, filename, offset));
}

void removePictures(const std::vector<PictureListForDeletion> &pictures)
{
	execute(new RemovePictures(pictures));
}

void addPictures(const std::vector<PictureListForAddition> &pictures)
{
	execute(new AddPictures(pictures));
}

void removeDevice(int idx)
{
	execute(new RemoveDevice(idx));
}

void editDeviceNickname(int idx, const QString &nickname)
{
	execute(new EditDeviceNickname(idx, nickname));
}

void createFilterPreset(const QString &name, const FilterData &data)
{
	execute(new CreateFilterPreset(name, data));
}

void removeFilterPreset(int index)
{
	execute(new RemoveFilterPreset(index));
}

void editFilterPreset(int index, const FilterData &data)
{
	execute(new EditFilterPreset(index, data));
}

} // namespace Command
