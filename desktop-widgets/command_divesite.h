// SPDX-License-Identifier: GPL-2.0
// Note: this header file is used by the undo-machinery and should not be included elsewhere.

#ifndef COMMAND_DIVESITE_H
#define COMMAND_DIVESITE_H

#include "command_base.h"

#include <QVector>

// We put everything in a namespace, so that we can shorten names without polluting the global namespace
namespace Command {

class DeleteDiveSites : public Base {
public:
	DeleteDiveSites(const QVector<dive_site *> &sites);
private:
	bool workToBeDone() override;
	void undo() override;
	void redo() override;

	// For redo
	std::vector<dive_site *> sitesToRemove;

	// For undo
	std::vector<OwningDiveSitePtr> sitesToAdd;
};

class EditDiveSiteName : public Base {
public:
	EditDiveSiteName(dive_site *ds, const QString &name);
private:
	bool workToBeDone() override;
	void undo() override;
	void redo() override;

	dive_site *ds;
	QString value; // Value to be set
};

class EditDiveSiteDescription : public Base {
public:
	EditDiveSiteDescription(dive_site *ds, const QString &description);
private:
	bool workToBeDone() override;
	void undo() override;
	void redo() override;

	dive_site *ds;
	QString value; // Value to be set
};

} // namespace Command

#endif // COMMAND_DIVESITE_H
