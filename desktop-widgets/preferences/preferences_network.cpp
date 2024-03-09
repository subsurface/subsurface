// SPDX-License-Identifier: GPL-2.0
#include "preferences_network.h"
#include "ui_preferences_network.h"
#include "subsurfacewebservices.h"
#include "core/errorhelper.h"
#include "core/settings/qPrefProxy.h"
#include <QNetworkProxy>

PreferencesNetwork::PreferencesNetwork() : AbstractPreferencesWidget(tr("Network"),QIcon(":preferences-system-network-icon"), 10), ui(new Ui::PreferencesNetwork())
{
	ui->setupUi(this);

	ui->proxyType->clear();
	ui->proxyType->addItem(tr("No proxy"), QNetworkProxy::NoProxy);
	ui->proxyType->addItem(tr("System proxy"), QNetworkProxy::DefaultProxy);
	ui->proxyType->addItem(tr("HTTP proxy"), QNetworkProxy::HttpProxy);
	ui->proxyType->addItem(tr("SOCKS proxy"), QNetworkProxy::Socks5Proxy);
	ui->proxyType->setCurrentIndex(-1);

	connect(ui->proxyType, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &PreferencesNetwork::proxyType_changed);
}

PreferencesNetwork::~PreferencesNetwork()
{
	delete ui;
}

void PreferencesNetwork::refreshSettings()
{
	ui->proxyHost->setText(prefs.proxy_host);
	ui->proxyPort->setValue(prefs.proxy_port);
	ui->proxyAuthRequired->setChecked(prefs.proxy_auth);
	ui->proxyUsername->setText(prefs.proxy_user);
	ui->proxyPassword->setText(prefs.proxy_pass);
	ui->proxyType->setCurrentIndex(ui->proxyType->findData(prefs.proxy_type));
}

void PreferencesNetwork::syncSettings()
{
	auto proxy = qPrefProxy::instance();

	proxy->set_proxy_type(ui->proxyType->itemData(ui->proxyType->currentIndex()).toInt());
	proxy->set_proxy_host(ui->proxyHost->text());
	proxy->set_proxy_port(ui->proxyPort->value());
	proxy->set_proxy_auth(ui->proxyAuthRequired->isChecked());
	proxy->set_proxy_user(ui->proxyUsername->text());
	proxy->set_proxy_pass(ui->proxyPassword->text());
}

void PreferencesNetwork::proxyType_changed(int idx)
{
	if (idx == -1) {
		return;
	}

	int proxyType = ui->proxyType->itemData(idx).toInt();
	bool hpEnabled = (proxyType == QNetworkProxy::Socks5Proxy || proxyType == QNetworkProxy::HttpProxy);
	ui->proxyHost->setEnabled(hpEnabled);
	ui->proxyPort->setEnabled(hpEnabled);
	ui->proxyAuthRequired->setEnabled(hpEnabled);
	ui->proxyUsername->setEnabled(hpEnabled & ui->proxyAuthRequired->isChecked());
	ui->proxyPassword->setEnabled(hpEnabled & ui->proxyAuthRequired->isChecked());
	ui->proxyAuthRequired->setChecked(ui->proxyAuthRequired->isChecked());
}
