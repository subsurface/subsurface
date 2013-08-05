#include "modeldelegates.h"
#include "../dive.h"
#include "../divelist.h"
#include "starwidget.h"
#include "models.h"
#include "diveplanner.h"

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
#include <QStringListModel>

// Gets the index of the model in the currentRow and column.
// currCombo is defined below.
#define IDX( XX ) mymodel->index(currCombo.currRow, XX)

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
	connect(this, SIGNAL(closeEditor(QWidget*,QAbstractItemDelegate::EndEditHint)),
	this, SLOT(revertModelData(QWidget*, QAbstractItemDelegate::EndEditHint)));
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
	comboDelegate->view()->installEventFilter( const_cast<QObject*>(qobject_cast<const QObject*>(this)));
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
		if (object == currCombo.comboEditor){ // the 'LineEdit' part
			QKeyEvent *ev = static_cast<QKeyEvent*>(event);
			if(ev->key() == Qt::Key_Up || ev->key() == Qt::Key_Down){
				currCombo.comboEditor->showPopup();
			}
		}
		else{	// the 'Drop Down Menu' part.
			QKeyEvent *ev = static_cast<QKeyEvent*>(event);
			if(    ev->key() == Qt::Key_Enter || ev->key() == Qt::Key_Return
				|| ev->key() == Qt::Key_Tab   || ev->key() == Qt::Key_Backtab
				|| ev->key() == Qt::Key_Escape){
				// treat Qt as a silly little boy - pretending that the key_return nwas pressed on the combo,
				// instead of the list of choices. this can be extended later for
				// other imputs, like tab navigation and esc.
				QStyledItemDelegate::eventFilter(currCombo.comboEditor, event);
			}
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

struct RevertCylinderData{
	QString type;
	int pressure;
	int size;
} currCylinderData;

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

	// don't fuck the other data, jimmy.
	if ( mymodel->data(thisindex, CylindersModel::TYPE).toString() == currCombo.activeText){
		return;
	}

	mymodel->setData(IDX(CylindersModel::TYPE), currCombo.activeText, Qt::EditRole);
	mymodel->passInData(IDX(CylindersModel::WORKINGPRESS), tankPressure);
	mymodel->passInData(IDX(CylindersModel::SIZE), tankSize);
}

TankInfoDelegate::TankInfoDelegate(QObject* parent): ComboBoxDelegate(TankInfoModel::instance(), parent)
{
}

void TankInfoDelegate::revertModelData(QWidget* widget, QAbstractItemDelegate::EndEditHint hint)
{
	if (hint == QAbstractItemDelegate::NoHint || hint == QAbstractItemDelegate::RevertModelCache){
		CylindersModel *mymodel = qobject_cast<CylindersModel *>(currCombo.model);
		mymodel->setData(IDX(CylindersModel::TYPE), currCylinderData.type, Qt::EditRole);
		mymodel->passInData(IDX(CylindersModel::WORKINGPRESS), currCylinderData.pressure);
		mymodel->passInData(IDX(CylindersModel::SIZE), currCylinderData.size);
	}
}

QWidget* TankInfoDelegate::createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
	// ncreate editor needs to be called before because it will populate a few
	// things in the currCombo global var.
	QWidget *delegate = ComboBoxDelegate::createEditor(parent, option, index);
	CylindersModel *mymodel = qobject_cast<CylindersModel *>(currCombo.model);
	cylinder_t *cyl = mymodel->cylinderAt(index);
	currCylinderData.type = cyl->type.description;
	currCylinderData.pressure = cyl->type.workingpressure.mbar;
	currCylinderData.size = cyl->type.size.mliter;
	return delegate;
}

struct RevertWeightData {
	QString type;
	int weight;
} currWeight;

void WSInfoDelegate::revertModelData(QWidget* widget, QAbstractItemDelegate::EndEditHint hint)
{
	if (hint == QAbstractItemDelegate::NoHint || hint == QAbstractItemDelegate::RevertModelCache){
		WeightModel *mymodel = qobject_cast<WeightModel *>(currCombo.model);
		mymodel->setData(IDX(WeightModel::TYPE), currWeight.type, Qt::EditRole);
		mymodel->passInData(IDX(WeightModel::WEIGHT), currWeight.weight);
	}
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

	// don't set if it's the same as it was before setting.
	if (mymodel->data(thisindex, WeightModel::TYPE).toString() == currCombo.activeText){
		return;
	}
	mymodel->setData(IDX(WeightModel::TYPE), v, Qt::EditRole);
	mymodel->passInData(IDX(WeightModel::WEIGHT), grams);
	qDebug() << "Fixme, every weight is 0.0 grams. see:" << grams;
}

WSInfoDelegate::WSInfoDelegate(QObject* parent): ComboBoxDelegate(WSInfoModel::instance(), parent)
{
}

QWidget* WSInfoDelegate::createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
	/* First, call the combobox-create editor, it will setup our globals. */
	QWidget *editor = ComboBoxDelegate::createEditor(parent, option, index);
	WeightModel *mymodel = qobject_cast<WeightModel *>(currCombo.model);
	weightsystem_t *ws = mymodel->weightSystemAt(index);
	currWeight.type = ws->description;
	currWeight.weight = ws->weight.grams;
	return editor;
}

void AirTypesDelegate::revertModelData(QWidget* widget, QAbstractItemDelegate::EndEditHint hint)
{
}

void AirTypesDelegate::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const
{
	if (!index.isValid())
		return;
	QComboBox *combo = qobject_cast<QComboBox*>(editor);
	model->setData(index, QVariant(combo->currentText()));
}

AirTypesDelegate::AirTypesDelegate(QObject* parent) : ComboBoxDelegate(airTypes(), parent)
{
}
