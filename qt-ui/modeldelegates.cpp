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
#include <QKeyEvent>
#include <QAbstractItemView>

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

	QVariant value = index.model()->data(index, DiveTripModel::STAR_ROLE);

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

ComboBoxDelegate::ComboBoxDelegate(QAbstractItemModel *model, QObject* parent): QStyledItemDelegate(parent), model(model)
{
}

void ComboBoxDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const
{
	QComboBox *c = qobject_cast<QComboBox*>(editor);
	QString data = index.model()->data(index, Qt::DisplayRole).toString();
	int i = c->findText(data);
	if (i != -1)
		c->setCurrentIndex(i);
	else
		c->setEditText(data);
}

struct CurrSelected{
	QComboBox *comboEditor;
	int currRow;
	QString activeText;
	QAbstractItemModel *model;
} currCombo;

QWidget* ComboBoxDelegate::createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
	QComboBox *comboDelegate = new QComboBox(parent);
	comboDelegate->setModel(model);
	comboDelegate->setEditable(true);
	comboDelegate->setAutoCompletion(true);
	comboDelegate->setAutoCompletionCaseSensitivity(Qt::CaseInsensitive);
	comboDelegate->completer()->setCompletionMode(QCompleter::PopupCompletion);
	comboDelegate->lineEdit()->installEventFilter( const_cast<QObject*>(qobject_cast<const QObject*>(this)));
	connect(comboDelegate, SIGNAL(highlighted(QString)), this, SLOT(testActivation(QString)));
	currCombo.comboEditor = comboDelegate;
	currCombo.currRow = index.row();
	currCombo.model = const_cast<QAbstractItemModel*>(index.model());
	return comboDelegate;
}

void ComboBoxDelegate::testActivation(const QString& s)
{
	currCombo.activeText = s;
	setModelData(currCombo.comboEditor, currCombo.model, QModelIndex());
}

bool ComboBoxDelegate::eventFilter(QObject* object, QEvent* event)
{
	// Reacts on Key_UP and Key_DOWN to show the QComboBox - list of choices.
	if (event->type() == QEvent::KeyPress){
		QKeyEvent *ev = static_cast<QKeyEvent*>(event);
		if(ev->key() == Qt::Key_Up || ev->key() == Qt::Key_Down){
			currCombo.comboEditor->showPopup();
		}
	}

    return QStyledItemDelegate::eventFilter(object, event);
}

void ComboBoxDelegate::updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
	QRect defaultRect = option.rect;
	defaultRect.setX( defaultRect.x() -1);
	defaultRect.setY( defaultRect.y() -1);
	defaultRect.setWidth( defaultRect.width() + 2);
	defaultRect.setHeight( defaultRect.height() + 2);
    editor->setGeometry(defaultRect);
}

void TankInfoDelegate::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& thisindex) const
{
	CylindersModel *mymodel = qobject_cast<CylindersModel *>(currCombo.model);
	TankInfoModel *tanks = TankInfoModel::instance();
	QModelIndexList matches = tanks->match(tanks->index(0,0), Qt::DisplayRole, currCombo.activeText);
	int row;
	if (matches.isEmpty()) {
		// we need to add this
		tanks->insertRows(tanks->rowCount(), 1);
		tanks->setData(tanks->index(tanks->rowCount() -1, 0), currCombo.activeText);
		row = tanks->rowCount() - 1;
	} else {
		row = matches.first().row();
	}
	int tankSize = tanks->data(tanks->index(row, TankInfoModel::ML)).toInt();
	int tankPressure = tanks->data(tanks->index(row, TankInfoModel::BAR)).toInt();

	mymodel->setData(model->index(currCombo.currRow, CylindersModel::TYPE), currCombo.activeText, Qt::EditRole);
	mymodel->passInData(model->index(currCombo.currRow, CylindersModel::WORKINGPRESS), tankPressure);
	mymodel->passInData(model->index(currCombo.currRow, CylindersModel::SIZE), tankSize);
}

TankInfoDelegate::TankInfoDelegate(QObject* parent): ComboBoxDelegate(TankInfoModel::instance(), parent)
{
}

void WSInfoDelegate::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& thisindex) const
{
	WeightModel *mymodel = qobject_cast<WeightModel *>(currCombo.model);
	WSInfoModel *wsim = WSInfoModel::instance();
	QModelIndexList matches = wsim->match(wsim->index(0,0), Qt::DisplayRole, currCombo.activeText);
	int row;
	if (matches.isEmpty()) {
		// we need to add this puppy
		wsim->insertRows(wsim->rowCount(), 1);
		wsim->setData(wsim->index(wsim->rowCount() - 1, 0), currCombo.activeText);
		row = wsim->rowCount() - 1;
	} else {
		row = matches.first().row();
	}
	int grams = wsim->data(wsim->index(row, WSInfoModel::GR)).toInt();
	QVariant v = QString(currCombo.activeText);
	mymodel->setData(model->index(currCombo.currRow, WeightModel::TYPE), v, Qt::EditRole);
	mymodel->passInData(model->index(currCombo.currRow, WeightModel::WEIGHT), grams);
	qDebug() << "Fixme, every weigth is 0.0 grams. see:" << grams;
}

WSInfoDelegate::WSInfoDelegate(QObject* parent): ComboBoxDelegate(WSInfoModel::instance(), parent)
{
}
