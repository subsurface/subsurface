// SPDX-License-Identifier: GPL-2.0
#ifndef WINDOWTITLEUPDATE_H
#define WINDOWTITLEUPDATE_H

#include <QObject>

class WindowTitleUpdate : public QObject
{
	Q_OBJECT
signals:
	void updateTitle();
};

extern WindowTitleUpdate windowTitleUpdate;

#endif // WINDOWTITLEUPDATE_H
