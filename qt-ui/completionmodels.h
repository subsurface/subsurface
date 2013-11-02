#ifndef COMPLETIONMODELS_H
#define COMPLETIONMODELS_H

#include <QStringListModel>

class BuddyCompletionModel : public QStringListModel {
	Q_OBJECT
public:
	static BuddyCompletionModel* instance();
	void updateModel();
};

class DiveMasterCompletionModel : public QStringListModel {
	Q_OBJECT
public:
	static DiveMasterCompletionModel* instance();
	void updateModel();
};

class LocationCompletionModel : public QStringListModel {
	Q_OBJECT
public:
	static LocationCompletionModel* instance();
	void updateModel();
};

class SuitCompletionModel : public QStringListModel {
	Q_OBJECT
public:
	static SuitCompletionModel* instance();
	void updateModel();
};

class TagCompletionModel : public QStringListModel {
	Q_OBJECT
public:
	static TagCompletionModel* instance();
	void updateModel();
};

#endif
