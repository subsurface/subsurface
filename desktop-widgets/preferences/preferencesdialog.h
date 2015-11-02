#ifndef PREFERENCES_WIDGET_H
#define PREFERENCES_WIDGET_H

#include <QDialog>
#include "pref.h"

class AbstractPreferencesWidget;
class QListWidget;
class QStackedWidget;
class QDialogButtonBox;
class QAbstractButton;

class PreferencesDialog : public QDialog {
	Q_OBJECT
public:
	static PreferencesDialog* instance();
	virtual ~PreferencesDialog();
	void addPreferencePage(AbstractPreferencesWidget *page);
	void refreshPages();
	void emitSettingsChanged();
signals:
	void settingsChanged();
private:
	PreferencesDialog();
	void cancelRequested();
	void applyRequested(bool closeIt);
	void defaultsRequested();
	void buttonClicked(QAbstractButton *btn);
	QList<AbstractPreferencesWidget*> pages;
	QListWidget *pagesList;
	QStackedWidget *pagesStack;
	QDialogButtonBox *buttonBox;
};

#endif
