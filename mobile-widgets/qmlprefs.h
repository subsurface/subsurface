// SPDX-License-Identifier: GPL-2.0
#ifndef QMLPREFS_H
#define QMLPREFS_H

#include <QObject>


class QMLPrefs : public QObject {
	Q_OBJECT

public:
	QMLPrefs();
	~QMLPrefs();

	static QMLPrefs *instance();

public slots:

private:
	static QMLPrefs *m_instance;

signals:
};

#endif
