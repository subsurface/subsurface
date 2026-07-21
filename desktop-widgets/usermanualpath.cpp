// SPDX-License-Identifier: GPL-2.0
#include "desktop-widgets/usermanualpath.h"

#include <QFile>

#include "core/qthelper.h"

QUrl getPackagedUserManualUrl()
{
	QString searchPath = getSubsurfaceDataPath("Documentation");
	if (searchPath.size()) {
		QString lang = getUiLanguage();
		QString prefix = searchPath + "/user-manual";
		QFile manual(prefix + "_" + lang + ".html");
		if (!manual.exists())
			manual.setFileName(prefix + "_" + lang.left(2) + ".html");
		if (!manual.exists())
			manual.setFileName(prefix + ".html");
		if (manual.exists())
			return QUrl::fromLocalFile(manual.fileName());
	}
	return QUrl();
}

QUrl getUserManualUrl()
{
	QUrl packagedUrl = getPackagedUserManualUrl();
	if (packagedUrl.isValid())
		return packagedUrl;
	return QUrl(QStringLiteral("https://subsurface-divelog.org/subsurface-user-manual/"));
}
