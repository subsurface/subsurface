#ifndef PREFERENCES_H
#define PREFERENCES_H

#include <QDialog>
#include "pref.h"

#include "ui_preferences.h"

#ifndef Q_OS_ANDROID
	class QWebView;
#endif

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
	void syncSettings();
	void loadSettings();
	void restorePrefs();
	void rememberPrefs();
	void facebookLoggedIn();
	void facebookDisconnect();
private:
	explicit PreferencesDialog(QWidget *parent = 0, Qt::WindowFlags f = 0);
	void setUiFromPrefs();
	Ui::PreferencesDialog ui;
	struct preferences oldPrefs;
    #ifndef Q_OS_ANDROID
	QWebView *facebookWebView;
    #endif
};

#endif // PREFERENCES_H
