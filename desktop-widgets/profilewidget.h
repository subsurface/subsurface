// SPDX-License-Identifier: GPL-2.0
// The profile and its toolbars.

#ifndef PROFILEWIDGET_H
#define PROFILEWIDGET_H

#include "ui_profilewidget.h"

#include <vector>
#include <memory>

class ProfileWidget2;

class ProfileWidget : public QWidget {
	Q_OBJECT
public:
	ProfileWidget();
	~ProfileWidget();
	std::unique_ptr<ProfileWidget2> view;
	void plotCurrentDive();
	void setPlanState(const struct dive *d, int dc);
	void setEditState(const struct dive *d, int dc);
	void setEnabledToolbar(bool enabled);
private
slots:
	void unsetProfHR();
	void unsetProfTissues();
private:
	std::vector<QAction *> toolbarActions;
	Ui::ProfileWidget ui;
	void setDive(const struct dive *d);
};

#endif // PROFILEWIDGET_H
