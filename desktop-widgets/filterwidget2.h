#ifndef FILTERWIDGET_2_H
#define FILTERWIDGET_2_H

#include <QHideEvent>
#include <QShowEvent>

#include <vector>
#include <memory>

#include "ui_filterwidget2.h"
#include "core/divefilter.h"
#include "qt-models/filterconstraintmodel.h"

class FilterConstraintWidget;
class QMenu;

class FilterWidget2 : public QWidget {
	Q_OBJECT

public:
	explicit FilterWidget2(QWidget *parent = 0);
	~FilterWidget2();
	QString shownText();

protected:
	void hideEvent(QHideEvent *event) override;
	void showEvent(QShowEvent *event) override;

private slots:
	void clearFilter();
	void closeFilter();
	void updateFilter();
	void constraintAdded(const QModelIndex &parent, int first, int last);
	void constraintRemoved(const QModelIndex &parent, int first, int last);
	void constraintChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles);
	void constraintsReset();
	void updatePresetMenu();
	void on_addSetButton_clicked();

private:
	bool ignoreSignal;
	Ui::FilterWidget2 ui;
	FilterConstraintModel constraintModel;
	bool validFilter;
	void addConstraint(filter_constraint_type type);
	std::vector<std::unique_ptr<FilterConstraintWidget>> constraintWidgets;
	FilterData createFilterData() const;
	void setFilterData(const FilterData &filterData);
	void loadPreset(int index);
	std::unique_ptr<QMenu> loadFilterPresetMenu;
};

#endif
