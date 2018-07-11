// SPDX-License-Identifier: GPL-2.0
/* qt-gui.cpp */
/* Qt UI implementation */
#include "core/display.h"
#include "core/qthelper.h"

#include <QApplication>
#include <QDesktopWidget>
#include <QNetworkProxy>
#include <QLibraryInfo>


#include <QQuickWindow>
#include <QScreen>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QSortFilterProxyModel>
#include "mobile-widgets/qmlmanager.h"
#include "qt-models/gpslistmodel.h"
#include "qt-models/messagehandlermodel.h"
#include "core/settings/qPref.h"

#include "mobile-widgets/qml/kirigami/src/kirigamiplugin.h"

#include "core/ssrf.h"

void set_non_bt_addresses() {
#if defined(Q_OS_ANDROID)
	connectionListModel.addAddress("FTDI");
#elif defined(Q_OS_LINUX) // since this is in the else, it does NOT include Android
	connectionListModel.addAddress("/dev/ttyS0");
	connectionListModel.addAddress("/dev/ttyS1");
	connectionListModel.addAddress("/dev/ttyS2");
	connectionListModel.addAddress("/dev/ttyS3");
	// this makes debugging so much easier - use the simulator
	connectionListModel.addAddress("/tmp/ttyS1");
#endif
}

bool haveFilesOnCommandLine()
{
	return false;
}
