#ifndef PREFERENCES_DIALOG_H
#define PREFERENCES_DIALOG_H

#include <QDialog>
#include "../dive.h"
#include "../pref.h"

#include "ui_preferences.h"

class QAbstractButton;

class PreferencesDialog :public QDialog{
Q_OBJECT
public:
	static PreferencesDialog* instance();
	void showEvent(QShowEvent* );
signals:
	void settingsChanged();
public slots:
	void buttonClicked(QAbstractButton* button);
	void on_chooseFile_clicked();
	void syncSettings();
	void restorePrefs();
	void rememberPrefs();

private:
	explicit PreferencesDialog(QWidget* parent = 0, Qt::WindowFlags f = 0);
	void setUiFromPrefs();
	void setPrefsFromUi();
	void setUIFromSettings();
	Ui::PreferencesDialog ui;
	struct preferences oldPrefs;
};

#endif
