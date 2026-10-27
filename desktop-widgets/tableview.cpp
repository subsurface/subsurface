// SPDX-License-Identifier: GPL-2.0
#include "desktop-widgets/tableview.h"
#include "desktop-widgets/modeldelegates.h"

#include <QEvent>
#include <QHeaderView>
#include <QMenu>
#include <QPushButton>
#include <QSettings>
#include <QStyle>

TableView::TableView(QWidget *parent) : QGroupBox(parent)
{
	ui.setupUi(this);
	ui.tableView->setItemDelegate(new DiveListDelegate(this));

	connect(ui.tableView, &QTableView::clicked, this, &TableView::itemClicked);

	QFontMetrics fm(defaultModelFont());
	int text_ht = fm.height();

	metrics.icon = &defaultIconMetrics();

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

	QIcon plusIcon(":list-add-icon");
	plusBtn = new QPushButton(plusIcon, QString(), this);
	plusBtn->setFlat(true);

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
#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
	// with Qt 5.15, this leads to an inoperable button
	plusBtn->resize(btnSize, btnSize);
#endif
	connect(plusBtn, SIGNAL(clicked(bool)), this, SIGNAL(addButtonClicked()));

	// Give the checkable column-visibility actions that are attached to the
	// table's horizontal header (see Qt::ActionsContextMenu) a visible,
	// discoverable control, instead of relying solely on the header's
	// right-click menu. The same QAction objects are reused in this menu,
	// so toggling a column here behaves identically to toggling it from the
	// header context menu, including persistence of the setting.
	columnsMenu = new QMenu(this);
	connect(columnsMenu, &QMenu::aboutToShow, this, [this]() {
		columnsMenu->clear();
		columnsMenu->addActions(ui.tableView->horizontalHeader()->actions());
	});

	columnsBtn = new QPushButton(style()->standardIcon(QStyle::SP_FileDialogDetailedView), QString(), this);
	columnsBtn->setFlat(true);
	columnsBtn->setMenu(columnsMenu);
	columnsBtn->setToolTip(tr("Select displayed columns"));
	columnsBtn->setIconSize(QSize(iconSize, iconSize));
#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
	columnsBtn->resize(btnSize, btnSize);
#endif
	// Only tables whose header has column-visibility actions attached (e.g.
	// the cylinders table) should show this button; others (e.g. the
	// weights table) must look exactly as before.
	columnsBtn->hide();
	ui.tableView->horizontalHeader()->installEventFilter(this);
}

TableView::~TableView()
{
	QSettings s;
	s.beginGroup(objectName());
	// remove the old default
	bool oldDefault = (ui.tableView->columnWidth(0) == 30);
	for (int i = 1; oldDefault && ui.tableView->model() && i < ui.tableView->model()->columnCount(); i++) {
		if (ui.tableView->columnWidth(i) != 80)
			oldDefault = false;
	}
	if (oldDefault) {
		s.remove("");
	} else if (ui.tableView->model()) {
		for (int i = 0; i < ui.tableView->model()->columnCount(); i++) {
			if (ui.tableView->columnWidth(i) == defaultColumnWidth(i)) {
				s.remove(QString("colwidth%1").arg(i));
			} else {
				s.setValue(QString("colwidth%1").arg(i), ui.tableView->columnWidth(i));
			}
		}
	} else {
		qWarning("TableView %s without model", qPrintable(objectName()));
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

void TableView::fixButtonPosition()
{
	QStyleOptionGroupBox option;
	initStyleOption(&option);
	QRect labelRect = style()->subControlRect(QStyle::CC_GroupBox, &option, QStyle::SC_GroupBoxLabel, this);
	QRect contentsRect = style()->subControlRect(QStyle::CC_GroupBox, &option, QStyle::QStyle::SC_GroupBoxFrame, this);
	int right = contentsRect.width();
	plusBtn->setGeometry(right - plusBtn->width(), labelRect.y(), plusBtn->width(), labelRect.height());
	right -= plusBtn->width();
	if (columnsBtn->isVisible())
		columnsBtn->setGeometry(right - columnsBtn->width(), labelRect.y(), columnsBtn->width(), labelRect.height());
}

// We need to manually position the 'plus' on cylinder and weight.
void TableView::resizeEvent(QResizeEvent *event)
{
	fixButtonPosition();
	QWidget::resizeEvent(event);
}

void TableView::showEvent(QShowEvent *event)
{
	QWidget::showEvent(event);
	fixButtonPosition();
}

bool TableView::eventFilter(QObject *obj, QEvent *event)
{
	if (obj == ui.tableView->horizontalHeader() &&
	    (event->type() == QEvent::ActionAdded || event->type() == QEvent::ActionRemoved))
		updateColumnsButtonVisibility();
	return QGroupBox::eventFilter(obj, event);
}

// The column-selector button is only useful (and only shown) once actions
// have actually been attached to the horizontal header. This keeps tables
// without column actions, such as the weights table, unchanged.
void TableView::updateColumnsButtonVisibility()
{
	columnsBtn->setVisible(!ui.tableView->horizontalHeader()->actions().isEmpty());
	fixButtonPosition();
}

void TableView::edit(const QModelIndex &index)
{
	ui.tableView->edit(index);
}

int TableView::defaultColumnWidth(int col)
{
	int width;
	QString text = ui.tableView->model()->headerData(col, Qt::Horizontal).toString();
#if QT_VERSION < QT_VERSION_CHECK(5, 11, 0)
	int textSpace = defaultModelFontMetrics().width(text) + 4;
#else // QT 5.11 or newer
	int textSpace = defaultModelFontMetrics().horizontalAdvance(text) + 4;
#endif
	width = text.isEmpty() ? metrics.rm_col_width : textSpace + 4; // add small margin
#if defined(Q_OS_MAC)
	width += 10; // Mac needs more margin
#endif
	return width;
}

QTableView *TableView::view()
{
	return ui.tableView;
}
