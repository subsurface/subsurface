#include "facebook_integration.h"
#include "facebookconnectwidget.h"

#include <QDebug>

FacebookPlugin::FacebookPlugin(QObject* parent) :
	fbConnectWidget(new FacebookConnectWidget()),
	fbUploadDialog(new SocialNetworkDialog())
{
}

bool FacebookPlugin::isConnected()
{
	FacebookManager *instance = FacebookManager::instance();
	return instance->loggedIn();
}

void FacebookPlugin::requestLogin()
{
	fbConnectWidget->exec();
}

void FacebookPlugin::requestLogoff()
{
	FacebookManager::instance()->logout();
}

QString FacebookPlugin::socialNetworkIcon() const
{
	return QString();
}

QString FacebookPlugin::socialNetworkName() const
{
	return tr("Facebook");
}

void FacebookPlugin::requestUpload()
{
	FacebookManager *instance = FacebookManager::instance();
	if (instance->loggedIn())
		fbUploadDialog->exec();
}
