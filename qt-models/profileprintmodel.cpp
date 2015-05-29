#include "profileprintmodel.h"
#include "metrics.h"
#include "dive.h"
#include "divetripmodel.h"
#include "helpers.h"

ProfilePrintModel::ProfilePrintModel(QObject *parent)
{
}

void ProfilePrintModel::setDive(struct dive *divePtr)
{
	diveId = divePtr->id;
	// reset();
}

void ProfilePrintModel::setFontsize(double size)
{
	fontSize = size;
}

int ProfilePrintModel::rowCount(const QModelIndex &parent) const
{
	return 12;
}

int ProfilePrintModel::columnCount(const QModelIndex &parent) const
{
	return 5;
}

QVariant ProfilePrintModel::data(const QModelIndex &index, int role) const
{
	const int row = index.row();
	const int col = index.column();

	switch (role) {
	case Qt::DisplayRole: {
		struct dive *dive = get_dive_by_uniq_id(diveId);
		struct DiveItem di;
		di.diveId = diveId;

		const QString unknown = tr("unknown");

		// dive# + date, depth, location, duration
		if (row == 0) {
			if (col == 0)
				return tr("Dive #%1 - %2").arg(dive->number).arg(di.displayDate());
			if (col == 3) {
				QString unit = (get_units()->length == units::METERS) ? "m" : "ft";
				return tr("Max depth: %1 %2").arg(di.displayDepth()).arg(unit);
			}
		}
		if (row == 1) {
			if (col == 0)
				return QString(get_dive_location(dive));
			if (col == 3)
				return QString(tr("Duration: %1 min")).arg(di.displayDuration());
		}
		// headings
		if (row == 2) {
			if (col == 0)
				return tr("Gas used:");
			if (col == 2)
				return tr("Tags:");
			if (col == 3)
				return tr("SAC:");
			if (col == 4)
				return tr("Weights:");
		}
		// notes
		if (col == 0) {
			if (row == 6)
				return tr("Notes:");
			if (row == 7)
				return QString(dive->notes);
		}
		// more headings
		if (row == 4) {
			if (col == 0)
				return tr("Divemaster:");
			if (col == 1)
				return tr("Buddy:");
			if (col == 2)
				return tr("Suit:");
			if (col == 3)
				return tr("Viz:");
			if (col == 4)
				return tr("Rating:");
		}
		// values for gas, sac, etc...
		if (row == 3) {
			if (col == 0) {
				int added = 0;
				QString gas, gases;
				for (int i = 0; i < MAX_CYLINDERS; i++) {
					if (!is_cylinder_used(dive, i))
						continue;
					gas = dive->cylinder[i].type.description;
					gas += QString(!gas.isEmpty() ? " " : "") + gasname(&dive->cylinder[i].gasmix);
					// if has a description and if such gas is not already present
					if (!gas.isEmpty() && gases.indexOf(gas) == -1) {
						if (added > 0)
							gases += QString(" / ");
						gases += gas;
						added++;
					}
				}
				return gases;
			}
			if (col == 2) {
				char buffer[256];
				taglist_get_tagstring(dive->tag_list, buffer, 256);
				return QString(buffer);
			}
			if (col == 3)
				return di.displaySac();
			if (col == 4) {
				weight_t tw = { total_weight(dive) };
				return get_weight_string(tw, true);
			}
		}
		// values for DM, buddy, suit, etc...
		if (row == 5) {
			if (col == 0)
				return QString(dive->divemaster);
			if (col == 1)
				return QString(dive->buddy);
			if (col == 2)
				return QString(dive->suit);
			if (col == 3)
				return (dive->visibility) ? QString::number(dive->visibility).append(" / 5") : QString();
			if (col == 4)
				return (dive->rating) ? QString::number(dive->rating).append(" / 5") : QString();
		}
		return QString();
	}
	case Qt::FontRole: {
		QFont font;
		font.setPointSizeF(fontSize);
		if (row == 0 && col == 0) {
			font.setBold(true);
		}
		return QVariant::fromValue(font);
	}
	case Qt::TextAlignmentRole: {
		// everything is aligned to the left
		unsigned int align = Qt::AlignLeft;
		// align depth and duration right
		if (row < 2 && col == 4)
			align = Qt::AlignRight | Qt::AlignVCenter;
		return QVariant::fromValue(align);
	}
	} // switch (role)
	return QVariant();
}
