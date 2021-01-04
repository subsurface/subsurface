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
private:
	StatsView *view;
	StatsState state;
	QStringList var1List;
	QStringList binner1List;
	QStringList var2List;
	QStringList binner2List;
	StatsState::UIState uiState;	// Remember UI state so that we can interpret indexes
	void updateUi();

};

#endif
