// SPDX-License-Identifier: GPL-2.0
#ifndef QPREFANIMATIONS_H
#define QPREFANIMATIONS_H


#include <QObject>


class qPrefAnimations : public QObject {
	Q_OBJECT
	Q_PROPERTY(int animation_speed READ animationSpeed WRITE setAnimationSpeed NOTIFY animationSpeedChanged);

public:
	qPrefAnimations(QObject *parent = NULL) : QObject(parent) {};
	~qPrefAnimations() {};
	static qPrefAnimations *instance();

	// Load/Sync local settings (disk) and struct preference
	void loadSync(bool doSync);

public:
	int animationSpeed() const;

public slots:
	void setAnimationSpeed(int value);

signals:
	void animationSpeedChanged(int value);

private:
	const QString group = QStringLiteral("Animations");
	static qPrefAnimations *m_instance;


	// functions to load/sync variable with disk
	void diskAnimationSpeed(bool doSync);
};

#endif
