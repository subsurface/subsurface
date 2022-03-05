// SPDX-License-Identifier: GPL-2.0
/*
 * maintab.h
 *
 * header file for the main tab of Subsurface
 *
 */
#ifndef MAINTAB_H
#define MAINTAB_H

#include <QTabWidget>

#include "ui_maintab.h"
#include "core/dive.h"
#include "core/subsurface-qt/divelistnotifier.h"

class TabBase;
class MainTab : public QTabWidget {
	Q_OBJECT
public:
	MainTab(QWidget *parent = 0);
	void clearTabs();
	void nextInputField(QKeyEvent *event);
	void stealFocus();

public
slots:
	void updateDiveInfo();
	void escDetected();
	void colorsChanged();
private:
	Ui::MainTab ui;
	bool lastSelectedDive;
	int lastTabSelectedDive;
	int lastTabSelectedDiveTrip;
	QList<TabBase*> extraWidgets;
	void changeEvent(QEvent *ev) override;
	bool isDark;
};

#endif // MAINTAB_H
