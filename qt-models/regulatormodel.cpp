// SPDX-License-Identifier: GPL-2.0
#include "qt-models/regulatormodel.h"
#include "core/dive.h"
#include "core/gettextfromc.h"
#include "core/metrics.h"
#include "core/helpers.h"
#include "qt-models/regulatorinfomodel.h"

RegulatorModel::RegulatorModel(QObject *parent) : CleanerTableModel(parent),
changed(false),
rows(0)
{
setHeaderDataStrings(QStringList() << tr("") << tr("Type") << tr("Service interval time")
		<< tr("Dives btw. services") << tr("Last service") << tr("Next service"));
}

regulator_t *RegulatorModel::regulatorAt(const QModelIndex &index)
{
	return &displayed_dive.regulators[index.row()];
}

void RegulatorModel::clear()
{
	if (rows > 0) {
		beginRemoveRows(QModelIndex(), 0, rows - 1);
		endRemoveRows();
	}
}

void RegulatorModel::remove(const QModelIndex &index)
{
	if (index.column() != REMOVE) {
		return;
	}
	beginRemoveRows(QModelIndex(), index.row(), index.row()); // yah, know, ugly.
	rows--;
	remove_regulator(&displayed_dive, index.row());
	changed = true;
	endRemoveRows();
}

QVariant RegulatorModel::data(const QModelIndex &index, int role) const
{
	QVariant ret;
	if (!index.isValid() || index.row() >= MAX_REGULATORS)
		return ret;

	regulator_t *reg = &displayed_dive.regulators[index.row()];

	switch (role) {
	case Qt::FontRole:
		ret = defaultModelFont();
		break;
	case Qt::TextAlignmentRole:
		ret = Qt::AlignCenter;
		break;
	case Qt::DisplayRole:
	case Qt::EditRole:
		switch (index.column()) {
		case TYPE:
			ret = gettextFromC::instance()->tr(reg->description);
			break;
		case SERVICE_INTERVAL_TIME:
			ret = get_interval_time_string(reg->service_interval_time_months);
			break;
		case SERVICE_INTERVAL_DIVES:
			ret = QString("%1").arg(reg->service_interval_number_of_dives);
			break;
		case LAST_SERVICE:
			if (reg->last_service > 0)
			{
				QDateTime dt = QDateTime::fromMSecsSinceEpoch(reg->last_service*1000);
				ret = dt.toString("yyyy-MM-dd");
			} else {
				ret = gettextFromC::instance()->tr("YYYY-MM-DD");
			}
			break;
		case NEXT_SERVICE:
			if (reg->last_service > 0 && reg->last_service < get_times()
			    && reg->service_interval_number_of_dives > 0
			    && reg->service_interval_time_months > 0)
			{
				/* Service prediction computable */
				int months_left = 0, days_left = 0, dives_left = 0;
				/* QDateTime dive_date = QDateTime::currentDateTime();
				 * Use the time of the selected dive instead of
				 * the current time.
				 */
				QDateTime dive_date = QDateTime::fromMSecsSinceEpoch(get_times() * 1000);
				QDateTime next_service_date = QDateTime::fromMSecsSinceEpoch(reg->last_service * 1000)
				                                        .addMonths(reg->service_interval_time_months);
				QString time_left_string;
				QString dives_left_string;
				
				dives_left = reg->service_interval_number_of_dives - get_dives_since_service(reg, get_times());
				
				if (dives_left  == 1) {
					dives_left_string = QString("1 %1").arg(gettextFromC::instance()->tr("dive"));
				} else if (dives_left > 1) {
					dives_left_string = QString("%1 %2").arg(dives_left).arg(gettextFromC::instance()->tr("dives"));
				}
				
				if (dive_date.toMSecsSinceEpoch() < next_service_date.toMSecsSinceEpoch()
				    && dives_left > 0) {
					days_left = dive_date.daysTo(next_service_date);
					while (dive_date.date().month() < next_service_date.date().month() &&
					       days_left >= dive_date.date().daysInMonth()) {
						days_left -= dive_date.date().daysInMonth();
						dive_date = dive_date.addMonths(1);
						months_left++;
					}
					if (months_left == 1)
						time_left_string.append(QString("1 %1").arg(gettextFromC::instance()->tr("month")));
					else if (months_left > 1)
						time_left_string.append(QString("%1 %2").arg(months_left).arg(gettextFromC::instance()->tr("months")));
					if (months_left < 6) {
						if (days_left == 1)
							time_left_string.append(QString(" 1 %1").arg(gettextFromC::instance()->tr("day")));
						else if (days_left > 1)
							time_left_string.append(QString(" %1 %2").arg(days_left).arg(gettextFromC::instance()->tr("days")));
					}
					ret = QString("%1 / %2").arg(time_left_string).arg(dives_left_string);
				} else {
					ret = gettextFromC::instance()->tr("Now");
				}
			} else {
				ret = tr("N/A");
			}
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
			ret = tr("Clicking here will remove this regulator.");
		else if (index.column() == LAST_SERVICE)
			ret = tr("Define last service before the selected dive.");
		else if (index.column() == NEXT_SERVICE)
			ret = tr("Service status for the time of the selected dive.");
		break;
	}
	return ret;
}

// this is our magic 'pass data in' function that allows the delegate to get
// the data here without silly unit conversions;
// so we only implement the two columns we care about
void RegulatorModel::passInData(const QModelIndex &index, const QVariant &value)
{
	regulator_t *reg = &displayed_dive.regulators[index.row()];
	if (index.column() == SERVICE_INTERVAL_TIME) {
		if (reg->service_interval_time_months != value.toInt()) {
			reg->service_interval_time_months = value.toInt();
			dataChanged(index, index);
		}
	} else if (index.column() == SERVICE_INTERVAL_DIVES) {
		if (reg->service_interval_number_of_dives != value.toInt()) {
			reg->service_interval_number_of_dives = value.toInt();
			dataChanged(index, index);
		}
	} else if (index.column() == LAST_SERVICE) {
		if (reg->last_service != value.toLongLong()) {
			reg->last_service = value.toLongLong();
			dataChanged(index, index);
		}
	}
}


bool RegulatorModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
	QString vString = value.toString();
	QRegExp rx("d(\\d+)");
	regulator_t *reg = &displayed_dive.regulators[index.row()];
	switch (index.column()) {
	case TYPE:
		if (!value.isNull()) {
			if (!reg->description || gettextFromC::instance()->tr(reg->description) != vString) {
				// loop over translations to see if one matches
				int i = -1;
				while (reg_info[++i].name) {
					if (gettextFromC::instance()->tr(reg_info[i].name) == vString) {
						reg->description = copy_string(reg_info[i].name);
						break;
					}
				}
				if (reg_info[i].name == NULL) // didn't find a match
					reg->description = copy_qstring(vString);
				changed = true;
			}
		}
		break;
	case SERVICE_INTERVAL_TIME:
		if (CHANGED()) {
			if (vString.contains(" "))
				vString.split(" ")[0];
			bool ok;
			int months = vString.toInt(&ok);
			if (ok && months > 0 && months <= 72) {
				reg->service_interval_time_months = months;
				changed = true;
				RegInfoModel *regim = RegInfoModel::instance();
				QModelIndexList matches = regim->match(regim->index(0, 0), Qt::DisplayRole, gettextFromC::instance()->tr(reg->description));
				if (!matches.isEmpty()) {
					regim->setData(regim->index(matches.first().row(), RegInfoModel::SERVICE_INTERVAL_TIME), reg->service_interval_time_months);
				}
			}
		}
		break;
	case SERVICE_INTERVAL_DIVES:
		if (CHANGED()) {
			if (vString.contains(" "))
				vString.split(" ")[0];
			bool ok;
			int number_of_dives = vString.toInt(&ok);
			if (ok && number_of_dives > 0) {
				reg->service_interval_number_of_dives = number_of_dives;
				changed = true;
				RegInfoModel *regim = RegInfoModel::instance();
				QModelIndexList matches = regim->match(regim->index(0, 0), Qt::DisplayRole, gettextFromC::instance()->tr(reg->description));
				if (!matches.isEmpty()) {
					regim->setData(regim->index(matches.first().row(), RegInfoModel::SERVICE_INTERVAL_DIVES), reg->service_interval_number_of_dives);
				}
			}
		}
		break;
	case LAST_SERVICE:
		if (CHANGED()) {
			QDateTime dt = string_to_date(qPrintable(vString));
			reg->last_service = dt.toMSecsSinceEpoch() / 1000;
			changed = true;
			RegInfoModel *regim = RegInfoModel::instance();
			QModelIndexList matches = regim->match(regim->index(0, 0), Qt::DisplayRole, gettextFromC::instance()->tr(reg->description));
			if (!matches.isEmpty()) {
				regim->setData(regim->index(matches.first().row(), RegInfoModel::LAST_SERVICE), (qlonglong) reg->last_service);
			}
		}
		break;
	case NEXT_SERVICE:
		//Display only variable
		break;
	}
	if (changed)
		dataChanged(index, index);
	return true;
}

Qt::ItemFlags RegulatorModel::flags(const QModelIndex &index) const
{
	if (index.column() == REMOVE)
		return Qt::ItemIsEnabled;
	return QAbstractItemModel::flags(index) | Qt::ItemIsEditable;
}

int RegulatorModel::rowCount(const QModelIndex &parent) const
{
	Q_UNUSED(parent);
	return rows;
}

void RegulatorModel::add()
{
	if (rows >= MAX_REGULATORS)
		return;

	int row = rows;
	beginInsertRows(QModelIndex(), row, row);
	rows++;
	changed = true;
	endInsertRows();
}

void RegulatorModel::updateDive()
{
	clear();
	rows = 0;
	for (int i = 0; i < MAX_REGULATORS; i++) {
		if (!regulator_none(&displayed_dive.regulators[i])) {
			rows = i + 1;
		}
	}
	if (rows > 0) {
		beginInsertRows(QModelIndex(), 0, rows - 1);
		endInsertRows();
	}
}
