// SPDX-License-Identifier: GPL-2.0
#ifndef TABLEVIEW_H
#define TABLEVIEW_H

/* This TableView is prepared to have the CSS,
 * the methods to restore / save the state of
 * the column widths and the 'plus' button.
 */
#include <QWidget>

#include "ui_tableview.h"

#include "core/metrics.h"

class QPushButton;
class QAbstractItemModel;
class QModelIndex;
class QTableView;
class QMenu;

class TableView : public QGroupBox {
	Q_OBJECT

	struct TableMetrics {
		const IconMetrics* icon; // icon metrics
		int rm_col_width; // column width of REMOVE column
		int header_ht; // height of the header
	};
public:
	TableView(QWidget *parent = 0);
	~TableView();
	void setModel(QAbstractItemModel *model);
	void setBtnToolTip(const QString &tooltip);
	void fixButtonPosition();
	void edit(const QModelIndex &index);
	int  defaultColumnWidth(int col); // default column width for column col
	QTableView *view();

protected:
	void showEvent(QShowEvent *) override;
	void resizeEvent(QResizeEvent *) override;
	bool eventFilter(QObject *obj, QEvent *event) override;

signals:
	void addButtonClicked();
	void itemClicked(const QModelIndex &);

private:
	void updateColumnsButtonVisibility();

	Ui::TableView ui;
	QPushButton *plusBtn;
	// Shows the checkable column-visibility actions that are attached to the
	// table's horizontal header (see Qt::ActionsContextMenu), so that hiding
	// a column remains discoverable without having to right-click the header.
	// Only shown when the header actually has such actions attached.
	QPushButton *columnsBtn;
	QMenu *columnsMenu;
	TableMetrics metrics;
};

#endif // TABLEVIEW_H
