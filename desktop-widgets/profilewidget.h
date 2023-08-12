// SPDX-License-Identifier: GPL-2.0
// The profile and its toolbars.

#ifndef PROFILEWIDGET_H
#define PROFILEWIDGET_H

#include "ui_profilewidget.h"
#include "core/subsurface-qt/divelistnotifier.h"

#include <vector>

struct dive;
class ProfileView;
class EmptyView;
class QQuickWidget;
class QStackedWidget;

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
	void exitEditMode();
	void dropPicture(const QString &filename, QPoint p);
	dive *d;
	int dc;
private
slots:
	void divesChanged(const QVector<dive *> &dives, DiveField field);
	void cylindersChanged(struct dive *changed, int pos);
	void unsetProfHR();
	void unsetProfTissues();
	void stopAdded();
	void stopRemoved(int count);
	void stopMoved(int count);
	void stopEdited();
private:
	ProfileView *getView();
	std::unique_ptr<QQuickWidget> viewWidget;
	std::unique_ptr<EmptyView> emptyView;
	std::vector<QAction *> toolbarActions;
	Ui::ProfileWidget ui;
	QStackedWidget *stack;
	void setDive(const struct dive *d, int dcNr);
	void editDive();
	void rotateDC(int dir);
	std::unique_ptr<dive> editedDive;
	bool placingCommand;
};

#endif // PROFILEWIDGET_H
