#include "cylindermodel.h"
#include "tankinfomodel.h"
#include "models.h"
#include "helpers.h"
#include "dive.h"
#include "color.h"
#include "diveplannermodel.h"
#include "gettextfromc.h"

CylindersModel::CylindersModel(QObject *parent) : changed(false),
	rows(0)
{
	//	enum {REMOVE, TYPE, SIZE, WORKINGPRESS, START, END, O2, HE, DEPTH};
	setHeaderDataStrings(QStringList() << "" << tr("Type") << tr("Size") << tr("Work press.") << tr("Start press.") << tr("End press.") << tr("O₂%") << tr("He%")
						 << tr("Switch at") << tr("Use"));

}

CylindersModel *CylindersModel::instance()
{

	static QScopedPointer<CylindersModel> self(new CylindersModel());
	return self.data();
}

static QVariant percent_string(fraction_t fraction)
{
	int permille = fraction.permille;

	if (!permille)
		return QVariant();
	return QString("%1%").arg(permille / 10.0, 0, 'f', 1);
}

QVariant CylindersModel::data(const QModelIndex &index, int role) const
{
	QVariant ret;

	if (!index.isValid() || index.row() >= MAX_CYLINDERS)
		return ret;

	cylinder_t *cyl = &displayed_dive.cylinder[index.row()];
	switch (role) {
	case Qt::BackgroundRole: {
		switch (index.column()) {
		// mark the cylinder start / end pressure in red if the values
		// seem implausible
		case START:
		case END:
			if ((cyl->start.mbar && !cyl->end.mbar) ||
					(cyl->end.mbar && cyl->start.mbar <= cyl->end.mbar))
				ret = REDORANGE1_HIGH_TRANS;
			break;
		}
		break;
	}
	case Qt::FontRole: {
		QFont font = defaultModelFont();
		switch (index.column()) {
		case START:
			font.setItalic(!cyl->start.mbar);
			break;
		case END:
			font.setItalic(!cyl->end.mbar);
			break;
		}
		ret = font;
		break;
	}
	case Qt::TextAlignmentRole:
		ret = Qt::AlignCenter;
		break;
	case Qt::DisplayRole:
	case Qt::EditRole:
		switch (index.column()) {
		case TYPE:
			ret = QString(cyl->type.description);
			break;
		case SIZE:
			if (cyl->type.size.mliter)
				ret = get_volume_string(cyl->type.size, true, cyl->type.workingpressure.mbar);
			break;
		case WORKINGPRESS:
			if (cyl->type.workingpressure.mbar)
				ret = get_pressure_string(cyl->type.workingpressure, true);
			break;
		case START:
			if (cyl->start.mbar)
				ret = get_pressure_string(cyl->start, true);
			else if (cyl->sample_start.mbar)
				ret = get_pressure_string(cyl->sample_start, true);
			break;
		case END:
			if (cyl->end.mbar)
				ret = get_pressure_string(cyl->end, true);
			else if (cyl->sample_end.mbar)
				ret = get_pressure_string(cyl->sample_end, true);
			break;
		case O2:
			ret = percent_string(cyl->gasmix.o2);
			break;
		case HE:
			ret = percent_string(cyl->gasmix.he);
			break;
		case DEPTH:
			ret = get_depth_string(cyl->depth, true);
			break;
		case USE:
			ret = gettextFromC::instance()->trGettext(cylinderuse_text[cyl->cylinder_use]);
			break;
		}
		break;
	case Qt::DecorationRole:
		if (index.column() == REMOVE)
			ret = trashIcon();
		break;
	case Qt::SizeHintRole:
		if (index.column() == REMOVE)
			ret = trashIcon().size();
		break;

	case Qt::ToolTipRole:
		if (index.column() == REMOVE)
			ret = tr("Clicking here will remove this cylinder.");
		break;
	}

	return ret;
}

cylinder_t *CylindersModel::cylinderAt(const QModelIndex &index)
{
	return &displayed_dive.cylinder[index.row()];
}

// this is our magic 'pass data in' function that allows the delegate to get
// the data here without silly unit conversions;
// so we only implement the two columns we care about
void CylindersModel::passInData(const QModelIndex &index, const QVariant &value)
{
	cylinder_t *cyl = cylinderAt(index);
	switch (index.column()) {
	case SIZE:
		if (cyl->type.size.mliter != value.toInt()) {
			cyl->type.size.mliter = value.toInt();
			dataChanged(index, index);
		}
		break;
	case WORKINGPRESS:
		if (cyl->type.workingpressure.mbar != value.toInt()) {
			cyl->type.workingpressure.mbar = value.toInt();
			dataChanged(index, index);
		}
		break;
	}
}

bool CylindersModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
	QString vString;
	bool addDiveMode = DivePlannerPointsModel::instance()->currentMode() != DivePlannerPointsModel::NOTHING;
	if (addDiveMode)
		DivePlannerPointsModel::instance()->rememberTanks();

	cylinder_t *cyl = cylinderAt(index);
	switch (index.column()) {
	case TYPE:
		if (!value.isNull()) {
			QByteArray ba = value.toByteArray();
			const char *text = ba.constData();
			if (!cyl->type.description || strcmp(cyl->type.description, text)) {
				cyl->type.description = strdup(text);
				changed = true;
			}
		}
		break;
	case SIZE:
		if (CHANGED()) {
			TankInfoModel *tanks = TankInfoModel::instance();
			QModelIndexList matches = tanks->match(tanks->index(0, 0), Qt::DisplayRole, cyl->type.description);

			cyl->type.size = string_to_volume(vString.toUtf8().data(), cyl->type.workingpressure);
			mark_divelist_changed(true);
			if (!matches.isEmpty())
				tanks->setData(tanks->index(matches.first().row(), TankInfoModel::ML), cyl->type.size.mliter);
			changed = true;
		}
		break;
	case WORKINGPRESS:
		if (CHANGED()) {
			TankInfoModel *tanks = TankInfoModel::instance();
			QModelIndexList matches = tanks->match(tanks->index(0, 0), Qt::DisplayRole, cyl->type.description);
			cyl->type.workingpressure = string_to_pressure(vString.toUtf8().data());
			if (!matches.isEmpty())
				tanks->setData(tanks->index(matches.first().row(), TankInfoModel::BAR), cyl->type.workingpressure.mbar / 1000.0);
			changed = true;
		}
		break;
	case START:
		if (CHANGED()) {
			cyl->start = string_to_pressure(vString.toUtf8().data());
			changed = true;
		}
		break;
	case END:
		if (CHANGED()) {
			//&& (!cyl->start.mbar || string_to_pressure(vString.toUtf8().data()).mbar <= cyl->start.mbar)) {
			cyl->end = string_to_pressure(vString.toUtf8().data());
			changed = true;
		}
		break;
	case O2:
		if (CHANGED()) {
			cyl->gasmix.o2 = string_to_fraction(vString.toUtf8().data());
			pressure_t modpO2;
			if (displayed_dive.dc.divemode == PSCR)
				modpO2.mbar = prefs.decopo2 + (1000 - get_o2(&cyl->gasmix)) * SURFACE_PRESSURE *
						prefs.o2consumption / prefs.decosac / prefs.pscr_ratio;
			else
				modpO2.mbar = prefs.decopo2;
			cyl->depth = gas_mod(&cyl->gasmix, modpO2, M_OR_FT(3, 10));
			changed = true;
		}
		break;
	case HE:
		if (CHANGED()) {
			cyl->gasmix.he = string_to_fraction(vString.toUtf8().data());
			changed = true;
		}
		break;
	case DEPTH:
		if (CHANGED()) {
			cyl->depth = string_to_depth(vString.toUtf8().data());
			changed = true;
		}
		break;
	case USE:
		if (CHANGED()) {
			cyl->cylinder_use = (enum cylinderuse)vString.toInt();
			changed = true;
		}
		break;
	}
	if (addDiveMode)
		DivePlannerPointsModel::instance()->tanksUpdated();
	dataChanged(index, index);
	return true;
}

int CylindersModel::rowCount(const QModelIndex &parent) const
{
	return rows;
}

void CylindersModel::add()
{
	if (rows >= MAX_CYLINDERS) {
		return;
	}

	int row = rows;
	fill_default_cylinder(&displayed_dive.cylinder[row]);
	displayed_dive.cylinder[row].manually_added = true;
	beginInsertRows(QModelIndex(), row, row);
	rows++;
	changed = true;
	endInsertRows();
}

void CylindersModel::clear()
{
	if (rows > 0) {
		beginRemoveRows(QModelIndex(), 0, rows - 1);
		endRemoveRows();
	}
}

void CylindersModel::updateDive()
{
	clear();
	rows = 0;
	for (int i = 0; i < MAX_CYLINDERS; i++) {
		if (!cylinder_none(&displayed_dive.cylinder[i]) &&
				(prefs.display_unused_tanks ||
				 is_cylinder_used(&displayed_dive, i) ||
				 displayed_dive.cylinder[i].manually_added))
			rows = i + 1;
	}
	if (rows > 0) {
		beginInsertRows(QModelIndex(), 0, rows - 1);
		endInsertRows();
	}
}

void CylindersModel::copyFromDive(dive *d)
{
	if (!d)
		return;
	rows = 0;
	for (int i = 0; i < MAX_CYLINDERS; i++) {
		if (!cylinder_none(&d->cylinder[i]) &&
				(is_cylinder_used(d, i) || prefs.display_unused_tanks)) {
			rows = i + 1;
		}
	}
	if (rows > 0) {
		beginInsertRows(QModelIndex(), 0, rows - 1);
		endInsertRows();
	}
}

Qt::ItemFlags CylindersModel::flags(const QModelIndex &index) const
{
	if (index.column() == REMOVE)
		return Qt::ItemIsEnabled;
	return QAbstractItemModel::flags(index) | Qt::ItemIsEditable;
}

void CylindersModel::remove(const QModelIndex &index)
{
	int mapping[MAX_CYLINDERS];
	if (index.column() != REMOVE) {
		return;
	}
	int same_gas = -1;
	cylinder_t *cyl = &displayed_dive.cylinder[index.row()];
	struct gasmix *mygas = &cyl->gasmix;
	for (int i = 0; i < MAX_CYLINDERS; i++) {
		mapping[i] = i;
		if (i == index.row() || cylinder_none(&displayed_dive.cylinder[i]))
			continue;
		struct gasmix *gas2 = &displayed_dive.cylinder[i].gasmix;
		if (gasmix_distance(mygas, gas2) == 0)
			same_gas = i;
	}
	if (same_gas == -1 &&
			((DivePlannerPointsModel::instance()->currentMode() != DivePlannerPointsModel::NOTHING &&
				DivePlannerPointsModel::instance()->tankInUse(cyl->gasmix)) ||
			 (DivePlannerPointsModel::instance()->currentMode() == DivePlannerPointsModel::NOTHING &&
				is_cylinder_used(&displayed_dive, index.row())))) {
				emit warningMessage(TITLE_OR_TEXT(
															tr("Cylinder cannot be removed"),
															tr("This gas is in use. Only cylinders that are not used in the dive can be removed.")));
		return;
	}
	beginRemoveRows(QModelIndex(), index.row(), index.row()); // yah, know, ugly.
	rows--;
	// if we didn't find an identical gas, point same_gas at the index.row()
	if (same_gas == -1)
		same_gas = index.row();
	if (index.row() == 0) {
		// first gas - we need to make sure that the same gas ends up
		// as first gas
		memmove(cyl, &displayed_dive.cylinder[same_gas], sizeof(*cyl));
		remove_cylinder(&displayed_dive, same_gas);
		mapping[same_gas] = 0;
		for (int i = same_gas + 1; i < MAX_CYLINDERS; i++)
			mapping[i] = i - 1;
	} else {
		remove_cylinder(&displayed_dive, index.row());
		if (same_gas > index.row())
			same_gas--;
		mapping[index.row()] = same_gas;
		for (int i = index.row() + 1; i < MAX_CYLINDERS; i++)
			mapping[i] = i - 1;
	}
	changed = true;
	endRemoveRows();
	struct divecomputer *dc = &displayed_dive.dc;
	while (dc) {
		dc_cylinder_renumber(&displayed_dive, dc, mapping);
		dc = dc->next;
	}
}
