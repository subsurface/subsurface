#ifndef FILTERWIDGET_H
#define FILTERWIDGET_H

#include <vector>
#include <memory>

#include "ui_filterwidget.h"
#include "core/divefilter.h"
#include "qt-models/filterconstraintmodel.h"

class FilterConstraintWidget;
class QMenu;
class QHideEvent;
class QShowEvent;

class FilterWidget : public QWidget {
	Q_OBJECT

public:
	explicit FilterWidget(QWidget *parent = 0);
	~FilterWidget();

protected:
	void hideEvent(QHideEvent *event) override;
	void showEvent(QShowEvent *event) override;

private slots:
	void clearFilter();
	void closeFilter();
	void filterChanged();
	void constraintAdded(const QModelIndex &parent, int first, int last);
	void constraintRemoved(const QModelIndex &parent, int first, int last);
	void constraintChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles);
	void constraintsReset();
	void updatePresetMenu();
	void presetClicked(const QModelIndex &index);
	void presetSelected(const QItemSelection &selected, const QItemSelection &);
	void on_addSetButton_clicked();

private:
	bool ignoreSignal;
	bool presetModified;
	Ui::FilterWidget ui;
	FilterConstraintModel constraintModel;
	void addConstraint(filter_constraint_type type);
	std::vector<std::unique_ptr<FilterConstraintWidget>> constraintWidgets;
	FilterData createFilterData() const;
	void updateFilter();
	void setFilterData(const FilterData &filterData);
	void loadPreset(int index);
	void selectPreset(int i);
	void clearFilterData();
	std::unique_ptr<QMenu> loadFilterPresetMenu;
	int selectedPreset() const; // returns -1 of no preset is selected
	void updatePresetLabel();
};

#endif
