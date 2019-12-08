// SPDX-License-Identifier: GPL-2.0
#ifndef PREFERENCES_MEDIA_H
#define PREFERENCES_MEDIA_H

#include <QMap>
#include "abstractpreferenceswidget.h"

namespace Ui {
	class PreferencesMedia;
}

class PreferencesMedia : public AbstractPreferencesWidget {
	Q_OBJECT
public:
	PreferencesMedia();
	~PreferencesMedia();
	void refreshSettings() override;
	void syncSettings() override;
public slots:
	void on_ffmpegFile_clicked();
	void on_ffmpegExecutable_editingFinished();
	void on_extractVideoThumbnails_toggled(bool toggled);
private:
	Ui::PreferencesMedia *ui;
	void checkFfmpegExecutable();

};

#endif

