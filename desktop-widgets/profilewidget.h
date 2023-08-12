// SPDX-License-Identifier: GPL-2.0
// The profile and its toolbars.

#ifndef PROFILEWIDGET_H
#define PROFILEWIDGET_H

#include "ui_profilewidget.h"
#include "core/owning_ptrs.h"
#include "core/subsurface-qt/divelistnotifier.h"

#include <vector>

struct dive;
class ProfileView;
class EmptyView;
class QQuickWidget;
class QStackedWidget;

extern "C" void free_dive(struct dive *);

class ProfileWidget : public QWidget {
	Q_OBJECT
public:
	ProfileWidget();
	~ProfileWidget();
	void plotDive(struct dive *d, int dc); // Attempt to keep DC number id dc < 0
	void plotCurrentDive();
	void setPlanState(const struct dive *d, int dc);
	void setEnabledToolbar(bool enabled);
	void nextDC();
	void prevDC();
	void dropPicture(const QString &filename, QPoint p);
	dive *d;
	int dc;
private
slots:
	void divesChanged(const QVector<dive *> &dives, DiveField field);
	void unsetProfHR();
	void unsetProfTissues();
	void stopAdded();
	void stopRemoved(int count);
	void stopMoved(int count);
private:
	ProfileView *getView();
	std::unique_ptr<QQuickWidget> viewWidget;
	std::unique_ptr<EmptyView> emptyView;
	std::vector<QAction *> toolbarActions;
	Ui::ProfileWidget ui;
	QStackedWidget *stack;
	void setDive(const struct dive *d);
	void editDive();
	void exitEditMode();
	void rotateDC(int dir);
	OwningDivePtr editedDive;
	int editedDc;
	dive *originalDive;
	bool placingCommand;
};

#endif // PROFILEWIDGET_H
