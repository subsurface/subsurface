// SPDX-License-Identifier: GPL-2.0
// The profile and its toolbars.

#ifndef DEPTHMAPWIDGET_H
#define DEPTHMAPWIDGET_H

#include "ui_depthmapwidget.h"

#include <vector>
#include <memory>

class ProfileWidget2;

#ifndef EMPTYVIEW
#define EMPTYVIEW
class EmptyView;
#endif

class QStackedWidget;

class DepthMapWidget : public QWidget {
	Q_OBJECT
public:
	DepthMapWidget();
	~DepthMapWidget();
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
	//std::unique_ptr<EmptyView> emptyView;
	std::vector<QAction *> toolbarActions;
	Ui::DepthMapWidget ui;
	QStackedWidget *stack;
	void setDive(const struct dive *d);
};

#endif // DEPTHMAPWIDGET_H
