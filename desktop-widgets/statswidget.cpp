// SPDX-License-Identifier: GPL-2.0
#include "statswidget.h"
#include "mainwindow.h"
#include "stats/statsview.h"
#include <QCheckBox>
#include <QPainter>
#include <QStyledItemDelegate>
#include <QQmlEngine>

class ChartItemDelegate : public QStyledItemDelegate {
private:
	void paint(QPainter *painter, const QStyleOptionViewItem &option,
		   const QModelIndex &index) const override;
	QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;
};

// Number of pixels that non-toplevel items are indented
static int indent(const QFontMetrics &fm)
{
#if QT_VERSION < QT_VERSION_CHECK(5, 11, 0)
	return 4 * fm.width('-');
#else
	return 4 * fm.horizontalAdvance('-');
#endif
}

static const int iconSpace = 2; // Number of pixels between icon and text
static const int topSpace = 2; // Number of pixels above icon

void ChartItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
			      const QModelIndex &index) const
{
	QFont font = index.data(Qt::FontRole).value<QFont>();
	QFontMetrics fm(font);
	QString name = index.data(ChartListModel::ChartNameRole).value<QString>();
	painter->setFont(font);
	QRect rect = option.rect;
	if (option.state & QStyle::State_Selected) {
		painter->save();
		painter->setBrush(option.palette.highlight());
		painter->setPen(Qt::NoPen);
		painter->drawRect(rect);
		painter->restore();
	}
	bool isHeader = index.data(ChartListModel::IsHeaderRole).value<bool>();
	if (!isHeader)
		rect.translate(indent(fm), 0);
	QPixmap icon = index.data(ChartListModel::IconRole).value<QPixmap>();
	if (!icon.isNull()) {
		rect.translate(0, topSpace);
		painter->drawPixmap(rect.topLeft(), icon);
		rect.translate(icon.size().width() + iconSpace, (icon.size().height() - fm.height()) / 2);
	}

	painter->drawText(rect, name);
}

QSize ChartItemDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	QFont font = index.data(Qt::FontRole).value<QFont>();
	QFontMetrics fm(font);
	QString name = index.data(ChartListModel::ChartNameRole).value<QString>();
	QSize iconSize = index.data(ChartListModel::IconSizeRole).value<QSize>();
	QSize size = fm.size(Qt::TextSingleLine, name);
	bool isHeader = index.data(ChartListModel::IsHeaderRole).value<bool>();
	if (!isHeader)
		size += QSize(indent(fm), 0);
	if (iconSize.isValid())
		size = QSize(size.width() + iconSize.width() + iconSpace,
			     std::max(size.height(), iconSize.height()) + 2 * topSpace);
	return size;
}

static const QUrl urlStatsView = QUrl(QStringLiteral("qrc:/qml/statsview2.qml"));
StatsWidget::StatsWidget(QWidget *parent) : QWidget(parent)
{
	ui.setupUi(this);

	connect(ui.close, &QToolButton::clicked, this, &StatsWidget::closeStats);

	ui.chartType->setModel(&charts);
	connect(ui.chartType, QOverload<int>::of(&QComboBox::activated), this, &StatsWidget::chartTypeChanged);
	connect(ui.var1, QOverload<int>::of(&QComboBox::activated), this, &StatsWidget::var1Changed);
	connect(ui.var2, QOverload<int>::of(&QComboBox::activated), this, &StatsWidget::var2Changed);
	connect(ui.var1Binner, QOverload<int>::of(&QComboBox::activated), this, &StatsWidget::var1BinnerChanged);
	connect(ui.var2Binner, QOverload<int>::of(&QComboBox::activated), this, &StatsWidget::var2BinnerChanged);
	connect(ui.var2Operation, QOverload<int>::of(&QComboBox::activated), this, &StatsWidget::var2OperationChanged);
	connect(ui.var1Sort, QOverload<int>::of(&QComboBox::activated), this, &StatsWidget::var1SortChanged);
	connect(ui.restrictButton, &QToolButton::clicked, this, &StatsWidget::restrict);
	connect(ui.unrestrictButton, &QToolButton::clicked, this, &StatsWidget::unrestrict);

	ui.stats->setSource(urlStatsView);
	ui.stats->setResizeMode(QQuickWidget::SizeRootObjectToView);
	(void)getView();
}

// hack around the Qt6 bug where the QML object gets destroyed and recreated
StatsView *StatsWidget::getView()
{
	StatsView *view = qobject_cast<StatsView *>(ui.stats->rootObject());
	if (!view)
		qWarning("Oops. The root of the StatsView is not a StatsView.");
	if (view) {
		// try to prevent the JS garbage collection from freeing the object
		// this appears to fail with Qt6 which is why we still look up the
		// object from the ui.stats rootObject
		ui.stats->engine()->setObjectOwnership(view, QQmlEngine::CppOwnership);
		view->setParent(this);
		view->setVisible(isVisible()); // Synchronize visibility of widget and QtQuick-view.
	}
	return view;
}

// Initialize QComboBox with list of variables
static void setVariableList(QComboBox *combo, const StatsState::VariableList &list)
{
	combo->clear();
	combo->setEnabled(!list.variables.empty());
	for (const StatsState::Variable &v: list.variables)
		combo->addItem(v.name, QVariant(v.id));
	combo->setCurrentIndex(list.selected);
}

// Initialize QComboBox and QLabel of binners. Hide if there are no binners.
static void setBinList(QComboBox *combo, const StatsState::BinnerList &list)
{
	combo->clear();
	combo->setEnabled(!list.binners.empty());

	for (const QString &s: list.binners)
		combo->addItem(s);
	combo->setCurrentIndex(list.selected);
}

void StatsWidget::updateUi()
{
	StatsState::UIState uiState = state.getUIState();
	setVariableList(ui.var1, uiState.var1);
	setVariableList(ui.var2, uiState.var2);
	int pos = charts.update(uiState.charts);
	ui.chartType->setCurrentIndex(pos);
	ui.chartType->setItemDelegate(new ChartItemDelegate);
	setBinList(ui.var1Binner, uiState.binners1);
	setBinList(ui.var2Binner, uiState.binners2);
	setVariableList(ui.var2Operation, uiState.operations2);
	setVariableList(ui.var1Sort, uiState.sortMode1);
	ui.sortGroup->setVisible(!uiState.sortMode1.variables.empty());

	// Add checkboxes for additional features
	features.clear();
	for (const StatsState::Feature &f: uiState.features) {
		features.emplace_back(new QCheckBox(f.name));
		QCheckBox *check = features.back().get();
		check->setChecked(f.selected);
		int id = f.id;
		connect(check, &QCheckBox::stateChanged, [this,id] (int state) { featureChanged(id, state); });
		ui.features->addWidget(check);
	}
	StatsView *view = getView();
	if (view)
		view->plot(state);
}

void StatsWidget::updateRestrictionLabel()
{
	StatsView *view = getView();
	if (!view)
		return;
	int num = view->restrictionCount();
	if (num < 0)
		ui.restrictionLabel->setText(tr("Analyzing all dives"));
	else
		ui.restrictionLabel->setText(tr("Analyzing subset (%L1) dives").arg(num));
	ui.unrestrictButton->setEnabled(num > 0);
}

void StatsWidget::closeStats()
{
	MainWindow::instance()->setApplicationState(MainWindow::ApplicationState::Default);
}

void StatsWidget::chartTypeChanged(int idx)
{
	state.chartChanged(ui.chartType->itemData(idx).toInt());
	updateUi();
}

void StatsWidget::var1Changed(int idx)
{
	state.var1Changed(ui.var1->itemData(idx).toInt());
	updateUi();
}

void StatsWidget::var2Changed(int idx)
{
	state.var2Changed(ui.var2->itemData(idx).toInt());
	updateUi();
}

void StatsWidget::var1BinnerChanged(int idx)
{
	state.binner1Changed(idx);
	updateUi();
}

void StatsWidget::var2BinnerChanged(int idx)
{
	state.binner2Changed(idx);
	updateUi();
}

void StatsWidget::var2OperationChanged(int idx)
{
	state.var2OperationChanged(ui.var2Operation->itemData(idx).toInt());
	updateUi();
}

void StatsWidget::var1SortChanged(int idx)
{
	state.sortMode1Changed(ui.var1Sort->itemData(idx).toInt());
	updateUi();
}

void StatsWidget::featureChanged(int idx, bool status)
{
	state.featureChanged(idx, status);
	// No need for a full chart replot - just show/hide the features
	StatsView *view = getView();
	if (view)
		view->updateFeatures(state);
}

void StatsWidget::showEvent(QShowEvent *e)
{
	unrestrict();
	updateUi();
	QWidget::showEvent(e);
	// Apparently, we have to manage the visibility of the view ourselves. That's mad.
	// this is implicitly done in getView() - so we can ignore the return value
	(void)getView();
}

void StatsWidget::hideEvent(QHideEvent *e)
{
	QWidget::hideEvent(e);
	// Apparently, we have to manage the visibility of the view ourselves. That's mad.
	// this is implicitly done in getView() - so we can ignore the return value
	(void)getView();
}

void StatsWidget::restrict()
{
	StatsView *view = getView();
	if (view)
		view->restrictToSelection();
	updateRestrictionLabel();
}

void StatsWidget::unrestrict()
{
	StatsView *view = getView();
	if (view)
		view->unrestrict();
	updateRestrictionLabel();
}
