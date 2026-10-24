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
	// AI-generated (Claude): offers to save a copy of the file wherever the user picks
	void saveFile(const QString &path);
private:
	void *self;
};

#endif /* IOSSHARE_H */
