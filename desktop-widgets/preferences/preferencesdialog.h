#ifndef PREFERENCES_WIDGET_H
#define PREFERENCES_WIDGET_H

#include <QDialog>
#include "pref.h"

class AbstractPreferencesWidget;
class QListWidget;
class QStackedWidget;
class QDialogButtonBox;

class PreferencesDialogV2 : public QDialog {
	Q_OBJECT
public:
	PreferencesDialogV2();
	virtual ~PreferencesDialogV2();
	void addPreferencePage(AbstractPreferencesWidget *page);
	void refreshPages();
private:
	void cancelRequested();
	void applyRequested();
	void defaultsRequested();

	QList<AbstractPreferencesWidget*> pages;
	QListWidget *pagesList;
	QStackedWidget *pagesStack;
	QDialogButtonBox *buttonBox;
};

#endif