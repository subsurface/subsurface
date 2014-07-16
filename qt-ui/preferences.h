#ifndef PREFERENCES_H
#define PREFERENCES_H

#include <QDialog>
#include "pref.h"

#include "ui_preferences.h"

class QAbstractButton;

class PreferencesDialog : public QDialog {
	Q_OBJECT
public:
	static PreferencesDialog *instance();
	void showEvent(QShowEvent *);
	void emitSettingsChanged();

signals:
	void settingsChanged();
public
slots:
	void buttonClicked(QAbstractButton *button);
	void on_chooseFile_clicked();
	void on_resetSettings_clicked();
	void syncSettings();
	void loadSettings();
	void restorePrefs();
	void rememberPrefs();
	void gflowChanged(int gf);
	void gfhighChanged(int gf);
	void proxyType_changed(int idx);
	void on_btnUseDefaultFile_toggled(bool toggle);

private:
	explicit PreferencesDialog(QWidget *parent = 0, Qt::WindowFlags f = 0);
	void setUiFromPrefs();
	Ui::PreferencesDialog ui;
	struct preferences oldPrefs;
};

#endif // PREFERENCES_H
