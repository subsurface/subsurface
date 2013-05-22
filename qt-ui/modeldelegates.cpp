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
	TankInfoModel *model = new TankInfoModel;
	QString data = index.model()->data(index, Qt::DisplayRole).toString();
	qDebug() << "Tentando pegar " << data;
	comboDelegate->setModel(model);
	int i;
	for(i = 0; i < model->rowCount(); i++){
		if (model->data(model->index(i,0), Qt::DisplayRole).toString() == data){
			break;
		}
	}
	if (i != model->rowCount())
		comboDelegate->setCurrentIndex(i);
	return comboDelegate;
}

TankInfoDelegate::TankInfoDelegate(QObject* parent): QStyledItemDelegate(parent)
{
}
