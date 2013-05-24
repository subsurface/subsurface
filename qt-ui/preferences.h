#ifndef PREFERENCES_DIALOG_H
#define PREFERENCES_DIALOG_H

#include <QDialog>

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
	
private:
	explicit PreferencesDialog(QWidget* parent = 0, Qt::WindowFlags f = 0);
    Ui::PreferencesDialog* ui;
};

#endif