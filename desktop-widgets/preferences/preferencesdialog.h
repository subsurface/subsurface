#ifndef PREFERENCES_WIDGET_H
#define PREFERENCES_WIDGET_H

#include <QDialog>
#include "pref.h"

class AbstractPreferencesWidget;
class QListWidget;
class QStackedWidget;
class QDialogButtonBox;
class QAbstractButton;

class PreferencesDialogV2 : public QDialog {
	Q_OBJECT
public:
	PreferencesDialogV2();
	virtual ~PreferencesDialogV2();
	void addPreferencePage(AbstractPreferencesWidget *page);
	void refreshPages();
signals:
	void settingsChanged();
private:
	void cancelRequested();
	void applyRequested();
	void defaultsRequested();
	void buttonClicked(QAbstractButton *btn);
	QList<AbstractPreferencesWidget*> pages;
	QListWidget *pagesList;
	QStackedWidget *pagesStack;
	QDialogButtonBox *buttonBox;
};

#endif