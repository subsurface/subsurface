// SPDX-License-Identifier: GPL-2.0
#ifndef STATSMANAGER_H
#define STATSMANAGER_H

#include "stats/statsview.h"
#include "stats/statsstate.h"

#include <QStringList>

class StatsManager : public QObject {
	Q_OBJECT
public:
	Q_PROPERTY(QStringList var1List MEMBER var1List NOTIFY var1ListChanged)
	Q_PROPERTY(QStringList binner1List MEMBER binner1List NOTIFY binner1ListChanged)
	Q_PROPERTY(QStringList var2List MEMBER var2List NOTIFY var2ListChanged)
	Q_PROPERTY(QStringList binner2List MEMBER binner2List NOTIFY binner2ListChanged)
	Q_PROPERTY(int var1Index MEMBER var1Index NOTIFY var1IndexChanged)
	Q_PROPERTY(int binner1Index MEMBER binner1Index NOTIFY binner1IndexChanged)
	Q_PROPERTY(int var2Index MEMBER var2Index NOTIFY var2IndexChanged)
	Q_PROPERTY(int binner2Index MEMBER binner2Index NOTIFY binner2IndexChanged)

	StatsManager();
	~StatsManager();
	Q_INVOKABLE void init(StatsView *v, QObject *o);
	Q_INVOKABLE void doit();
	Q_INVOKABLE void var1Changed(int idx);
	Q_INVOKABLE void var1BinnerChanged(int idx);
	Q_INVOKABLE void var2Changed(int idx);
	Q_INVOKABLE void var2BinnerChanged(int idx);
signals:
	void var1ListChanged();
	void binner1ListChanged();
	void var2ListChanged();
	void binner2ListChanged();
	void var1IndexChanged();
	void binner1IndexChanged();
	void var2IndexChanged();
	void binner2IndexChanged();
private:
	StatsView *view;
	StatsState state;
	QStringList var1List;
	QStringList binner1List;
	QStringList var2List;
	QStringList binner2List;
	int var1Index;
	int binner1Index;
	int var2Index;
	int binner2Index;
	StatsState::UIState uiState;	// Remember UI state so that we can interpret indexes
	void updateUi();

};

#endif
