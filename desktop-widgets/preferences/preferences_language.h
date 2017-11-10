// SPDX-License-Identifier: GPL-2.0
#ifndef PREFERENCES_LANGUAGE_H
#define PREFERENCES_LANGUAGE_H

#include <QMap>
#include "abstractpreferenceswidget.h"

namespace Ui {
	class PreferencesLanguage;
}

class PreferencesLanguage : public AbstractPreferencesWidget {
	Q_OBJECT
public:
	PreferencesLanguage();
	virtual ~PreferencesLanguage();
	virtual void refreshSettings();
	virtual void syncSettings();
private:
	Ui::PreferencesLanguage *ui;
	QMap<QString, QString> dateFormatShortMap;
public slots:
	void dateFormatChanged(const QString&);
};

#endif
