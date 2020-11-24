// SPDX-License-Identifier: GPL-2.0
#ifndef PREFERENCES_WIDGET_H
#define PREFERENCES_WIDGET_H

#include <QDialog>

class AbstractPreferencesWidget;
class QListWidget;
class QStackedWidget;
class QDialogButtonBox;
class QAbstractButton;

class PreferencesDialog : public QDialog {
	Q_OBJECT
public:
	static PreferencesDialog *instance();
	~PreferencesDialog();
	void refreshPages();
	void defaultsRequested();
private:
	PreferencesDialog();
	void cancelRequested();
	void applyRequested(bool closeIt);
	void buttonClicked(QAbstractButton *btn);
	QList<AbstractPreferencesWidget*> pages;
	QListWidget *pagesList;
	QStackedWidget *pagesStack;
	QDialogButtonBox *buttonBox;
};

#endif
