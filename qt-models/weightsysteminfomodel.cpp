// SPDX-License-Identifier: GPL-2.0
#include "qt-models/weightsysteminfomodel.h"
#include "core/subsurface-qt/divelistnotifier.h"
#include "core/dive.h"
#include "core/metrics.h"
#include "core/gettextfromc.h"

QVariant WSInfoModel::data(const QModelIndex &index, int role) const
{
	if (!index.isValid() || index.row() >= rows)
		return QVariant();
	struct ws_info_t *info = &ws_info[index.row()];

	int gr = info->grams;
	switch (role) {
	case Qt::FontRole:
		return defaultModelFont();
	case Qt::DisplayRole:
	case Qt::EditRole:
		switch (index.column()) {
		case GR:
			return gr;
		case DESCRIPTION:
			return gettextFromC::tr(info->name);
		}
		break;
	}
	return QVariant();
}

int WSInfoModel::rowCount(const QModelIndex&) const
{
	return rows;
}

WSInfoModel::WSInfoModel(QObject *parent) : CleanerTableModel(parent)
{
	setHeaderDataStrings(QStringList() << tr("Description") << tr("kg"));
	rows = 0;
	for (struct ws_info_t *info = ws_info; info->name && info < ws_info + MAX_WS_INFO; info++, rows++)
		;
}
