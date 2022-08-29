// SPDX-License-Identifier: GPL-2.0
#ifndef IOSSHARE_H
#define IOSSHARE_H

// onlt Qt headers and data structures allowed here
#include <QString>

class IosShare {
public:
	IosShare();
	~IosShare();
	void supportEmail(const QString &firstPath, const QString &secondPath);
	void shareViaEmail(const QString &subject, const QString &recipient, const QString &body, const QString &firstPath, const QString &secondPath);
private:
	void *self;
};

#endif /* IOSSHARE_H */
