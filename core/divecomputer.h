// SPDX-License-Identifier: GPL-2.0
#ifndef DIVECOMPUTER_H
#define DIVECOMPUTER_H

#include <QString>
#include <QVector>
#include <stdint.h>

class DiveComputerNode {
public:
	bool operator==(const DiveComputerNode &a) const;
	bool operator!=(const DiveComputerNode &a) const;
	bool operator<(const DiveComputerNode &a) const;
	void showchanges(const QString &n, const QString &s, const QString &f) const;
	QString model;
	uint32_t deviceId;
	QString serialNumber;
	QString firmware;
	QString nickName;
};

class DiveComputerList {
public:
	const DiveComputerNode *getExact(const QString &m, uint32_t d);
	const DiveComputerNode *get(const QString &m);
	void addDC(QString m, uint32_t d, QString n = QString(), QString s = QString(), QString f = QString());

	// Keep the dive computers in a vector sorted by (model, deviceId)
	QVector<DiveComputerNode> dcs;
};

QString get_dc_nickname(const struct divecomputer *dc);
extern DiveComputerList dcList;

#endif
