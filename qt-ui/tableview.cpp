#include "tableview.h"
#include "models.h"
#include "modeldelegates.h"

#include <QPushButton>
#include <QLayout>
#include <QFile>
#include <QTextStream>
#include <QSettings>
#include <QStyle>

TableView::TableView(QWidget *parent) : QGroupBox(parent)
{
	ui.setupUi(this);
	ui.tableView->setItemDelegate(new DiveListDelegate(this));

	QFontMetrics fm(defaultModelFont());
	int text_ht = fm.height();
	int text_em = fm.width('m');

	metrics.icon = &defaultIconMetrics();

	metrics.col_width = 7*text_em;
	metrics.rm_col_width = metrics.icon->sz_small + 2*metrics.icon->spacing;
	metrics.header_ht = text_ht + 10; // TODO DPI

	/* We want to get rid of the margin around the table, but
	 * we must be careful with some styles (e.g. GTK+) where the top
	 * margin is actually used to hold the label. We thus check the
	 * rectangles for the label and contents to make sure they do not
	 * overlap, and adjust the top contentsMargin accordingly
	 */

	// start by setting all the margins at zero
	QMargins margins;

	// grab the label and contents dimensions and positions
	QStyleOptionGroupBox option;
	initStyleOption(&option);
	QRect labelRect = style()->subControlRect(QStyle::CC_GroupBox, &option, QStyle::SC_GroupBoxLabel, this);
	QRect contentsRect = style()->subControlRect(QStyle::CC_GroupBox, &option, QStyle::SC_GroupBoxContents, this);

	/* we need to ensure that the bottom of the label is higher
	 * than the top of the contents */
	int delta = contentsRect.top() - labelRect.bottom();
	const int min_gap = metrics.icon->spacing;
	if (delta <= min_gap) {
		margins.setTop(min_gap - delta);
	}
	layout()->setContentsMargins(margins);

	QIcon plusIcon(":plus");
	plusBtn = new QPushButton(plusIcon, QString(), this);
	plusBtn->setFlat(true);
	plusBtn->setToolTip(tr("Add cylinder"));

	/* now determine the icon and button size. Since the button will be
	 * placed in the label, make sure that we do not overflow, as it might
	 * get clipped
	 */
	int iconSize = metrics.icon->sz_small;
	int btnSize = iconSize + 2*min_gap;
	if (btnSize > labelRect.height()) {
		btnSize = labelRect.height();
		iconSize = btnSize - 2*min_gap;
	}
	plusBtn->setIconSize(QSize(iconSize, iconSize));
	plusBtn->resize(btnSize, btnSize);
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
	QStyleOptionGroupBox option;
	initStyleOption(&option);
	QRect labelRect = style()->subControlRect(QStyle::CC_GroupBox, &option, QStyle::SC_GroupBoxLabel, this);
	QRect contentsRect = style()->subControlRect(QStyle::CC_GroupBox, &option, QStyle::QStyle::SC_GroupBoxFrame, this);
	plusBtn->setGeometry( contentsRect.width() - plusBtn->width(), labelRect.y(), plusBtn->width(), labelRect.height());
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
