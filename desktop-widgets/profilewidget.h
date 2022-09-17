// SPDX-License-Identifier: GPL-2.0
// The profile and its toolbars.

#ifndef PROFILEWIDGET_H
#define PROFILEWIDGET_H

#include "ui_profilewidget.h"
#include "core/subsurface-qt/divelistnotifier.h"

#include <vector>
#include <memory>

struct dive;
class ProfileWidget2;
class EmptyView;
class QStackedWidget;

extern "C" void free_dive(struct dive *);

class ProfileWidget : public QWidget {
	Q_OBJECT
public:
	ProfileWidget();
	~ProfileWidget();
	std::unique_ptr<ProfileWidget2> view;
	void plotDive(struct dive *d, int dc); // Attempt to keep DC number id dc < 0
	void plotCurrentDive();
	void setPlanState(const struct dive *d, int dc);
	void setEnabledToolbar(bool enabled);
	void nextDC();
	void prevDC();
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
	// The same code is in command/command_base.h. Should we make that a global feature?
	struct DiveDeleter {
		void operator()(dive *d) { free_dive(d); }
	};

	std::unique_ptr<EmptyView> emptyView;
	std::vector<QAction *> toolbarActions;
	Ui::ProfileWidget ui;
	QStackedWidget *stack;
	void setDive(const struct dive *d);
	void editDive();
	void exitEditMode();
	void rotateDC(int dir);
	std::unique_ptr<dive, DiveDeleter> editedDive;
	int editedDc;
	dive *originalDive;
	bool placingCommand;
};

#endif // PROFILEWIDGET_H
