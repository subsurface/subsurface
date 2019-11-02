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
	void fixPlusPosition();
	void edit(const QModelIndex &index);
	int  defaultColumnWidth(int col); // default column width for column col
	QTableView *view();

protected:
	void showEvent(QShowEvent *) override;
	void resizeEvent(QResizeEvent *) override;

signals:
	void addButtonClicked();
	void itemClicked(const QModelIndex &);

private:
	Ui::TableView ui;
	QPushButton *plusBtn;
	TableMetrics metrics;
};

#endif // TABLEVIEW_H
