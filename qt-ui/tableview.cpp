#include "tableview.h"
#include "models.h"
#include "modeldelegates.h"

#include <QPushButton>
#include <QLayout>
#include <QFile>
#include <QTextStream>
#include <QSettings>

TableView::TableView(QWidget *parent) : QWidget(parent)
{
	ui.setupUi(this);
	ui.tableView->setItemDelegate(new DiveListDelegate(this));

	QFontMetrics fm(defaultModelFont());
	int text_ht = fm.height();
	int text_em = fm.width('m');

	metrics.icon = &defaultIconMetrics();

	metrics.col_width = 7*text_em;
	metrics.rm_col_width = 3*text_em;
	metrics.header_ht = text_ht + 10; // TODO DPI

	/* There`s mostly a need for a Mac fix here too. */
	if (qApp->style()->objectName() == "gtk+")
		ui.groupBox->layout()->setContentsMargins(0, 9, 0, 0);
	else
		ui.groupBox->layout()->setContentsMargins(0, 0, 0, 0);

	QIcon plusIcon(":plus");
	plusBtn = new QPushButton(plusIcon, QString(), ui.groupBox);
	plusBtn->setFlat(true);
	plusBtn->setToolTip(tr("Add cylinder"));
	plusBtn->setIconSize(QSize(metrics.icon->sz_small, metrics.icon->sz_small));
	connect(plusBtn, SIGNAL(clicked(bool)), this, SIGNAL(addButtonClicked()));
}

TableView::~TableView()
{
	QSettings s;
	s.beginGroup(objectName());
	// remove the old default
	bool oldDefault = (ui.tableView->columnWidth(0) == 30);
	for (int i = 1; oldDefault && i < ui.tableView->model()->columnCount(); i++) {
		if (ui.tableView->columnWidth(i) != 80)
			oldDefault = false;
	}
	if (oldDefault) {
		s.remove("");
	} else {
		for (int i = 0; i < ui.tableView->model()->columnCount(); i++) {
			if (ui.tableView->columnWidth(i) == defaultColumnWidth(i))
				s.remove(QString("colwidth%1").arg(i));
			else
				s.setValue(QString("colwidth%1").arg(i), ui.tableView->columnWidth(i));
		}
	}
	s.endGroup();
}

void TableView::setBtnToolTip(const QString &tooltip)
{
	plusBtn->setToolTip(tooltip);
}

void TableView::setTitle(const QString &title)
{
	ui.groupBox->setTitle(title);
}

void TableView::setModel(QAbstractItemModel *model)
{
	ui.tableView->setModel(model);
	connect(ui.tableView, SIGNAL(clicked(QModelIndex)), model, SLOT(remove(QModelIndex)));

	QSettings s;
	s.beginGroup(objectName());
	const int columnCount = ui.tableView->model()->columnCount();
	for (int i = 0; i < columnCount; i++) {
		QVariant width = s.value(QString("colwidth%1").arg(i), defaultColumnWidth(i));
		ui.tableView->setColumnWidth(i, width.toInt());
	}
	s.endGroup();

	ui.tableView->horizontalHeader()->setMinimumHeight(metrics.header_ht);
}

void TableView::fixPlusPosition()
{
	int x = ui.groupBox->contentsRect().width() - 2*metrics.icon->sz_small + metrics.icon->spacing;
	int y = metrics.icon->spacing;
	int sz = metrics.icon->sz_med;
	plusBtn->setGeometry(x, y, sz, sz);
}

// We need to manually position the 'plus' on cylinder and weight.
void TableView::resizeEvent(QResizeEvent *event)
{
	fixPlusPosition();
	QWidget::resizeEvent(event);
}

void TableView::showEvent(QShowEvent *event)
{
	QWidget::showEvent(event);
	fixPlusPosition();
}

void TableView::edit(const QModelIndex &index)
{
	ui.tableView->edit(index);
}

int TableView::defaultColumnWidth(int col)
{
	return col == CylindersModel::REMOVE ? metrics.rm_col_width : metrics.col_width;
}

QTableView *TableView::view()
{
	return ui.tableView;
}
