#ifndef PREFERENCES_DIALOG_H
#define PREFERENCES_DIALOG_H

#include <QDialog>
#include "../dive.h"
#include "../pref.h"

namespace Ui{
	class PreferencesDialog;
}

class PreferencesDialog :public QDialog{
Q_OBJECT
public:
	static PreferencesDialog* instance();

signals:
	void settingsChanged();

public slots:
	void syncSettings();
	void resetSettings();

private:
	explicit PreferencesDialog(QWidget* parent = 0, Qt::WindowFlags f = 0);
	Ui::PreferencesDialog* ui;
	struct preferences oldPrefs;
};

#endif
