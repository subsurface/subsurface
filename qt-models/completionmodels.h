// SPDX-License-Identifier: GPL-2.0
#ifndef COMPLETIONMODELS_H
#define COMPLETIONMODELS_H

#include <QStringListModel>

class BuddyCompletionModel : public QStringListModel {
	Q_OBJECT
public:
	void updateModel();
};

class DiveMasterCompletionModel : public QStringListModel {
	Q_OBJECT
public:
	void updateModel();
};

class SuitCompletionModel : public QStringListModel {
	Q_OBJECT
public:
	void updateModel();
};

class TagCompletionModel : public QStringListModel {
	Q_OBJECT
public:
	void updateModel();
};

#endif // COMPLETIONMODELS_H
