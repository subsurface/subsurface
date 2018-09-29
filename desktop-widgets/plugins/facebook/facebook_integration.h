// SPDX-License-Identifier: GPL-2.0
#ifndef FACEBOOK_INTEGRATION_H
#define FACEBOOK_INTEGRATION_H

#include "core/isocialnetworkintegration.h"
#include <QString>

class FacebookConnectWidget;
class SocialNetworkDialog;
class FacebookManager;

class FacebookPlugin : public ISocialNetworkIntegration {
	Q_OBJECT
public:
	explicit FacebookPlugin(QObject* parent = 0);
	bool isConnected() override;
	void requestLogin() override;
	void requestLogoff() override;
	QString socialNetworkIcon() const override;
	QString socialNetworkName() const override;
	void requestUpload() override;
private:
	FacebookConnectWidget *fbConnectWidget;
};

#endif
