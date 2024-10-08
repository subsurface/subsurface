// SPDX-License-Identifier: GPL-2.0
// Note: this header file is used by the undo-machinery and should not be included elsewhere.

#ifndef COMMAND_DIVESITE_H
#define COMMAND_DIVESITE_H

#include "command_base.h"

#include <QVector>

struct DiveAndLocation {
	struct dive *d;
	location_t location;
	QString name;
};

// We put everything in a namespace, so that we can shorten names without polluting the global namespace
namespace Command {

class AddDiveSite : public Base {
public:
	AddDiveSite(const QString &name);
private:
	bool workToBeDone() override;
	void undo() override;
	void redo() override;

	// Note: we only add one dive site. Nevertheless, we use vectors so that we
	// can reuse the dive site deletion code.
	// For undo
	std::vector<dive_site *> sitesToRemove;

	// For redo
	std::vector<std::unique_ptr<dive_site>> sitesToAdd;
};

class ImportDiveSites : public Base {
public:
	// Note: Takes ownership of dive site table
	ImportDiveSites(dive_site_table sites, const QString &source);
private:
	bool workToBeDone() override;
	void undo() override;
	void redo() override;

	// For undo
	std::vector<dive_site *> sitesToRemove;

	// For redo
	std::vector<std::unique_ptr<dive_site>> sitesToAdd;
};

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
	std::vector<std::unique_ptr<dive_site>> sitesToAdd;
};

class PurgeUnusedDiveSites : public Base {
public:
	PurgeUnusedDiveSites();
private:
	bool workToBeDone() override;
	void undo() override;
	void redo() override;

	// For redo
	std::vector<dive_site *> sitesToRemove;

	// For undo
	std::vector<std::unique_ptr<dive_site>> sitesToAdd;
};

class EditDiveSiteName : public Base {
public:
	EditDiveSiteName(dive_site *ds, const QString &name);
private:
	bool workToBeDone() override;
	void undo() override;
	void redo() override;

	dive_site *ds;
	std::string value; // Value to be set
};

class EditDiveSiteDescription : public Base {
public:
	EditDiveSiteDescription(dive_site *ds, const QString &description);
private:
	bool workToBeDone() override;
	void undo() override;
	void redo() override;

	dive_site *ds;
	std::string value; // Value to be set
};

class EditDiveSiteNotes : public Base {
public:
	EditDiveSiteNotes(dive_site *ds, const QString &notes);
private:
	bool workToBeDone() override;
	void undo() override;
	void redo() override;

	dive_site *ds;
	std::string value; // Value to be set
};

class EditDiveSiteCountry : public Base {
public:
	EditDiveSiteCountry(dive_site *ds, const QString &country);
private:
	bool workToBeDone() override;
	void undo() override;
	void redo() override;

	dive_site *ds;
	std::string value; // Value to be set
};

class EditDiveSiteLocation : public Base {
public:
	EditDiveSiteLocation(dive_site *ds, location_t location);
private:
	bool workToBeDone() override;
	void undo() override;
	void redo() override;

	dive_site *ds;
	location_t value; // Value to be set
};

class EditDiveSiteTaxonomy : public Base {
public:
	EditDiveSiteTaxonomy(dive_site *ds, taxonomy_data &taxonomy);
	~EditDiveSiteTaxonomy(); // free taxonomy
private:
	bool workToBeDone() override;
	void undo() override;
	void redo() override;

	dive_site *ds;
	taxonomy_data value; // Value to be set
};

class MergeDiveSites : public Base {
public:
	MergeDiveSites(dive_site *ds, const QVector<dive_site *> &sites);
private:
	bool workToBeDone() override;
	void undo() override;
	void redo() override;

	dive_site *ds;

	// For redo
	std::vector<dive_site *> sitesToRemove;

	// For undo
	std::vector<std::unique_ptr<dive_site>> sitesToAdd;
};

class ApplyGPSFixes : public Base {
public:
	// Note: the dive site table is consumed after the call it will be empty.
	ApplyGPSFixes(const std::vector<DiveAndLocation> &fixes);
private:
	bool workToBeDone() override;
	void undo() override;
	void redo() override;

	// For undo
	std::vector<dive_site *> sitesToRemove;

	// For redo
	std::vector<std::unique_ptr<dive_site>> sitesToAdd;

	// For redo and undo
	struct SiteAndLocation {
		dive_site *ds;
		location_t location;
	};
	std::vector<SiteAndLocation> siteLocations;
	void editDiveSites();
};

} // namespace Command

#endif // COMMAND_DIVESITE_H
