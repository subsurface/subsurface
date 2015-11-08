#include "facebook_integration.h"

FacebookPlugin::FacebookPlugin(QObject* parent): QObject(parent)
{

}

bool FacebookPlugin::isConnected()
{
	return false;
}

void FacebookPlugin::requestLogin()
{

}

void FacebookPlugin::requestLogoff()
{

}

QString FacebookPlugin::socialNetworkIcon() const
{
	return QString();
}

QString FacebookPlugin::socialNetworkName() const
{
	return tr("Facebook");
}

void FacebookPlugin::uploadCurrentDive()
{

}
