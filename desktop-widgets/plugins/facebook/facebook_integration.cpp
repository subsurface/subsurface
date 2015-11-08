#include "facebook_integration.h"
#include "facebookconnectwidget.h"

#include <QDebug>

FacebookPlugin::FacebookPlugin(QObject* parent): QObject(parent)
{

}

bool FacebookPlugin::isConnected()
{
	return false;
}

void FacebookPlugin::requestLogin()
{
	FacebookConnectWidget connectDialog;
	connectDialog.exec();
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

void FacebookPlugin::requestUpload()
{
	qDebug() << "Upload Requested";
}
