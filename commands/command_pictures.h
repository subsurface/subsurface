// SPDX-License-Identifier: GPL-2.0
// Note: this header file is used by the undo-machinery and should not be included elsewhere.

#ifndef COMMAND_PICTURES_H
#define COMMAND_PICTURES_H

#include "command_base.h"
#include "command.h" // for PictureListForDeletion/Addition

// We put everything in a namespace, so that we can shorten names without polluting the global namespace
namespace Command {

class SetPictureOffset final : public Base {
public:
	SetPictureOffset(dive *d, const QString &filename, offset_t offset); // Pictures are identified by the unique (dive,filename) pair
private:
	dive *d; // null means no work to be done
	QString filename;
	offset_t offset;

	void undo() override;
	void redo() override;
	bool workToBeDone() override;
};

class RemovePictures final : public Base {
public:
	RemovePictures(const std::vector<PictureListForDeletion> &pictures);
private:
	std::vector<PictureListForDeletion> picturesToRemove; // for redo
	std::vector<PictureListForAddition> picturesToAdd; // for undo

	void undo() override;
	void redo() override;
	bool workToBeDone() override;
};

class AddPictures final : public Base {
public:
	AddPictures(const std::vector<PictureListForAddition> &pictures);
private:
	struct DiveSiteEntry {
		dive *d;
		dive_site *ds;
	};
	struct DiveSiteEditEntry {
		dive_site *ds;
		location_t location;
	};
	std::vector<PictureListForAddition> picturesToAdd; // for redo
	std::vector<std::unique_ptr<dive_site>> sitesToAdd; //for redo
	std::vector<PictureListForDeletion> picturesToRemove; // for undo
	std::vector<dive_site *> sitesToRemove; // for undo
	std::vector<DiveSiteEntry> sitesToSet; // for redo and undo
	std::vector<DiveSiteEditEntry> sitesToEdit; // for redo and undo

	void swapDiveSites();

	void undo() override;
	void redo() override;
	bool workToBeDone() override;
};

} // namespace Command
#endif
