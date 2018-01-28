// SPDX-License-Identifier: GPL-2.0
#include "notificationwidget.h"

NotificationWidget::NotificationWidget(QWidget *parent) : KMessageWidget(parent)
{
	connect(&future_watcher, SIGNAL(finished()), this, SLOT(finish()));
}

void NotificationWidget::showNotification(QString message, KMessageWidget::MessageType type)
{
	if (message.isEmpty())
		return;
	setText(message);
	setCloseButtonVisible(true);
	setMessageType(type);
	animatedShow();
}

void NotificationWidget::showError(QString message)
{
	showNotification(message, KMessageWidget::Error);
}

void NotificationWidget::hideNotification()
{
	animatedHide();
}

QString NotificationWidget::getNotificationText()
{
	return text();
}

void NotificationWidget::setFuture(const QFuture<void> &future)
{
	future_watcher.setFuture(future);
}

void NotificationWidget::finish()
{
	hideNotification();
}
