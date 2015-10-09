#ifndef PLUGINMANAGER_H
#define PLUGINMANAGER_H

#include <QObject>

#include "isocialnetworkintegration.h"

class PluginManager {
public:
	static PluginManager& instance();
	void loadPlugins();
	QList<ISocialNetworkIntegration*> socialNetworkIntegrationPlugins() const;
private:
	PluginManager();
	PluginManager(const PluginManager&) = delete;
	PluginManager& operator=(const PluginManager&) = delete;
};

#endif