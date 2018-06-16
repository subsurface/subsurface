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
	bool changesValues(const DiveComputerNode &b) const;
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
	DiveComputerNode matchDC(const QString &m, uint32_t d);
	DiveComputerNode matchModel(const QString &m);

	// Keep the dive computers in a vector sorted by (model, deviceId)
	QVector<DiveComputerNode> dcs;
};

extern DiveComputerList dcList;

#endif
