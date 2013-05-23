#include "modeldelegates.h"
#include "../dive.h"
#include "../divelist.h"
#include "starwidget.h"
#include "models.h"

#include <QtDebug>
#include <QPainter>
#include <QSortFilterProxyModel>
#include <QStyle>
#include <QStyleOption>
#include <QComboBox>
#include <QCompleter>
#include <QLineEdit>

StarWidgetsDelegate::StarWidgetsDelegate(QWidget* parent):
	QStyledItemDelegate(parent),
	parentWidget(parent)
{

}

void StarWidgetsDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
	QStyledItemDelegate::paint(painter, option, index);

	if (!index.isValid())
		return;

	QVariant value = index.model()->data(index, TreeItemDT::STAR_ROLE);

	if (!value.isValid())
		return;

	int rating = value.toInt();

	painter->save();
	painter->setRenderHint(QPainter::Antialiasing, true);

	for(int i = 0; i < rating; i++)
		painter->drawPixmap(option.rect.x() + i * IMG_SIZE + SPACING, option.rect.y(), StarWidget::starActive());

	for(int i = rating; i < TOTALSTARS; i++)
		painter->drawPixmap(option.rect.x() + i * IMG_SIZE + SPACING, option.rect.y(), StarWidget::starInactive());

	painter->restore();
}

QSize StarWidgetsDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
	return QSize(IMG_SIZE * TOTALSTARS + SPACING * (TOTALSTARS-1), IMG_SIZE);
}

QWidget* TankInfoDelegate::createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
	QComboBox *comboDelegate = new QComboBox(parent);
	TankInfoModel *model = TankInfoModel::instance();
	comboDelegate->setModel(model);
	comboDelegate->setEditable(true);
	comboDelegate->setAutoCompletion(true);
	comboDelegate->setAutoCompletionCaseSensitivity(Qt::CaseInsensitive);
	comboDelegate->completer()->setCompletionMode(QCompleter::PopupCompletion);
	return comboDelegate;
}

void TankInfoDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const
{
	QComboBox *c = qobject_cast<QComboBox*>(editor);
	QString data = index.model()->data(index, Qt::DisplayRole).toString();
	int i = c->findText(data);
	if (i != -1)
		c->setCurrentIndex(i);
	else
		c->setEditText(data);
}

void TankInfoDelegate::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const
{
	QComboBox *c = static_cast<QComboBox*>(editor);
	CylindersModel *mymodel = qobject_cast<CylindersModel *>(model);
	TankInfoModel *tanks = TankInfoModel::instance();
	QModelIndex tankIndex = tanks->match(tanks->index(0,0), Qt::DisplayRole, c->currentText()).first();

	int tankSize = tanks->data(tanks->index(tankIndex.row(), TankInfoModel::ML)).toInt();
	int tankPressure = tanks->data(tanks->index(tankIndex.row(), TankInfoModel::BAR)).toInt();

	mymodel->setData(index, c->currentText(), Qt::EditRole);
	mymodel->passInData(model->index(index.row(), CylindersModel::WORKINGPRESS), tankPressure);
	mymodel->passInData(model->index(index.row(), CylindersModel::SIZE), tankSize);
}

TankInfoDelegate::TankInfoDelegate(QObject* parent): QStyledItemDelegate(parent)
{
}
