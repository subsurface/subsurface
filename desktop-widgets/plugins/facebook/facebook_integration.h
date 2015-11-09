#ifndef FACEBOOK_INTEGRATION_H
#define FACEBOOK_INTEGRATION_H

#include "subsurface-core/isocialnetworkintegration.h"
#include <QString>

class FacebookConnectWidget;
class SocialNetworkDialog;
class FacebookManager;

class FacebookPlugin : public ISocialNetworkIntegration {
	Q_OBJECT
public:
	explicit FacebookPlugin(QObject* parent = 0);
	virtual bool isConnected();
	virtual void requestLogin();
	virtual void requestLogoff();
	virtual QString socialNetworkIcon() const;
	virtual QString socialNetworkName() const;
	virtual void requestUpload();
private:
	FacebookConnectWidget *fbConnectWidget;
	SocialNetworkDialog *fbUploadDialog;
};

#endif