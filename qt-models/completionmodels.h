// SPDX-License-Identifier: GPL-2.0
#ifndef COMPLETIONMODELS_H
#define COMPLETIONMODELS_H

#include "core/subsurface-qt/divelistnotifier.h"
#include <QStringListModel>

struct dive;

class CompletionModelBase : public QStringListModel {
	Q_OBJECT
public:
	CompletionModelBase();
private slots:
	void updateModel();
	void divesChanged(const QVector<dive *> &dives, DiveField field);
protected:
	virtual QStringList getStrings() = 0;
	virtual bool relevantDiveField(const DiveField &f) = 0;
};

class BuddyCompletionModel final : public CompletionModelBase {
	Q_OBJECT
private:
	QStringList getStrings() override;
	bool relevantDiveField(const DiveField &f) override;
};

class DiveMasterCompletionModel final : public CompletionModelBase {
	Q_OBJECT
private:
	QStringList getStrings() override;
	bool relevantDiveField(const DiveField &f) override;
};

class SuitCompletionModel final : public CompletionModelBase {
	Q_OBJECT
private:
	QStringList getStrings() override;
	bool relevantDiveField(const DiveField &f) override;
};

class TagCompletionModel final : public CompletionModelBase {
	Q_OBJECT
private:
	QStringList getStrings() override;
	bool relevantDiveField(const DiveField &f) override;
};

#endif // COMPLETIONMODELS_H
