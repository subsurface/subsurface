#include "modeldelegates.h"
#include "../dive.h"
#include "../divelist.h"
#include "starwidget.h"
#include "models.h"
#include "diveplanner.h"
#include "simplewidgets.h"

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
#include <QApplication>

QSize DiveListDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
	return QSize(50,22);
}

// Gets the index of the model in the currentRow and column.
// currCombo is defined below.
#define IDX(_XX) mymodel->index(currCombo.currRow, (_XX))
static bool keyboardFinished = false;

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
	int deltaY = option.rect.height() / 2 - StarWidget::starActive().height() / 2 ;
	painter->save();
	painter->setRenderHint(QPainter::Antialiasing, true);
	for (int i = 0; i < rating; i++)
		painter->drawPixmap(option.rect.x() + i * IMG_SIZE + SPACING, option.rect.y() + deltaY, StarWidget::starActive());
	for (int i = rating; i < TOTALSTARS; i++)
		painter->drawPixmap(option.rect.x() + i * IMG_SIZE + SPACING, option.rect.y() + deltaY, StarWidget::starInactive());
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

struct CurrSelected {
	QComboBox *comboEditor;
	int currRow;
	QString activeText;
	QAbstractItemModel *model;
	bool ignoreSelection;
} currCombo;

QWidget* ComboBoxDelegate::createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
	QComboBox *comboDelegate = new QComboBox(parent);
	comboDelegate->setModel(model);
	comboDelegate->setEditable(true);
	comboDelegate->setAutoCompletion(true);
	comboDelegate->setAutoCompletionCaseSensitivity(Qt::CaseInsensitive);
	comboDelegate->completer()->setCompletionMode(QCompleter::PopupCompletion);
	comboDelegate->view()->setEditTriggers(QAbstractItemView::AllEditTriggers);
	comboDelegate->lineEdit()->installEventFilter( const_cast<QObject*>(qobject_cast<const QObject*>(this)));
	comboDelegate->view()->installEventFilter( const_cast<QObject*>(qobject_cast<const QObject*>(this)));
	connect(comboDelegate, SIGNAL(highlighted(QString)), this, SLOT(testActivation(QString)));
	connect(comboDelegate, SIGNAL(activated(QString)), this, SLOT(fakeActivation()));
	connect(this, SIGNAL(closeEditor(QWidget*,QAbstractItemDelegate::EndEditHint)), this, SLOT(fixTabBehavior()));
	currCombo.comboEditor = comboDelegate;
	currCombo.currRow = index.row();
	currCombo.model = const_cast<QAbstractItemModel*>(index.model());
	keyboardFinished = false;

	// Current display of things on Gnome3 looks like shit, so
	// let`s fix that.
	if (isGnome3Session()) {
		QPalette p;
		p.setColor(QPalette::Window, QColor(Qt::white));
		p.setColor(QPalette::Base, QColor(Qt::white));
		comboDelegate->lineEdit()->setPalette(p);
		comboDelegate->setPalette(p);
	}
	return comboDelegate;
}

/* This Method is being called when the user *writes* something and press enter or tab,
 * and it`s also called when the mouse walks over the list of choices from the ComboBox,
 * One thing is important, if the user writes a *new* cylinder or weight type, it will
 * be ADDED to the list, and the user will need to fill the other data.
 */
void ComboBoxDelegate::testActivation(const QString& currText)
{
	currCombo.activeText = currText.isEmpty() ? currCombo.comboEditor->currentText() : currText;
	setModelData(currCombo.comboEditor, currCombo.model, QModelIndex());
}

// HACK, send a fake event so Qt thinks we hit 'enter' on the line edit.
void ComboBoxDelegate::fakeActivation() {
	/* this test is needed because as soon as I show the selector,
	 * the first item gots selected, this sending an activated signal,
	 * calling this fakeActivation code and setting as the current,
	 * thig that we don't want. so, let's just set the ignoreSelection
	 * to false and be happy, because the next activation  ( by click
	 * or keypress) is real.
	 */
	if (currCombo.ignoreSelection) {
		currCombo.ignoreSelection = false;
		return;
	}
	QKeyEvent ev(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier);
	QStyledItemDelegate::eventFilter(currCombo.comboEditor, &ev);
}

// This 'reverts' the model data to what we actually choosed,
// becaus e a TAB is being understood by Qt as 'cancel' while
// we are on a QComboBox ( but not on a QLineEdit.
void ComboBoxDelegate::fixTabBehavior()
{
	if (keyboardFinished) {
		setModelData(0,0,QModelIndex());
	}
}

bool ComboBoxDelegate::eventFilter(QObject* object, QEvent* event)
{
	// Reacts on Key_UP and Key_DOWN to show the QComboBox - list of choices.
	if (event->type() == QEvent::KeyPress || event->type() == QEvent::ShortcutOverride) {
		if (object == currCombo.comboEditor) { // the 'LineEdit' part
			QKeyEvent *ev = static_cast<QKeyEvent*>(event);
			if (ev->key() == Qt::Key_Up || ev->key() == Qt::Key_Down) {
				currCombo.ignoreSelection = true;
				currCombo.comboEditor->showPopup();
			}
			if (ev->key() == Qt::Key_Tab || ev->key() == Qt::Key_Enter || ev->key() == Qt::Key_Return) {
				currCombo.activeText  = currCombo.comboEditor->currentText();
				keyboardFinished = true;
			}
		} else {	// the 'Drop Down Menu' part.
			QKeyEvent *ev = static_cast<QKeyEvent*>(event);
			if (ev->key() == Qt::Key_Enter || ev->key() == Qt::Key_Return ||
			    ev->key() == Qt::Key_Tab   || ev->key() == Qt::Key_Backtab ||
			    ev->key() == Qt::Key_Escape) {
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

struct RevertCylinderData {
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

	mymodel->setData(IDX(CylindersModel::TYPE), currCombo.activeText, Qt::EditRole);
	mymodel->passInData(IDX(CylindersModel::WORKINGPRESS), tankPressure);
	mymodel->passInData(IDX(CylindersModel::SIZE), tankSize);
}

TankInfoDelegate::TankInfoDelegate(QObject* parent): ComboBoxDelegate(TankInfoModel::instance(), parent)
{
}

void TankInfoDelegate::revertModelData(QWidget* widget, QAbstractItemDelegate::EndEditHint hint)
{
	if (
#if !defined __APPLE__
	    hint == QAbstractItemDelegate::NoHint ||
#endif
	    hint == QAbstractItemDelegate::RevertModelCache) {
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
	if (
#if !defined __APPLE__
	    hint == QAbstractItemDelegate::NoHint ||
#endif
	    hint == QAbstractItemDelegate::RevertModelCache) {
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

	mymodel->setData(IDX(WeightModel::TYPE), v, Qt::EditRole);
	mymodel->passInData(IDX(WeightModel::WEIGHT), grams);
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

AirTypesDelegate::AirTypesDelegate(QObject* parent) : ComboBoxDelegate(GasSelectionModel::instance(), parent)
{
}

ProfilePrintDelegate::ProfilePrintDelegate(QObject *parent)
	: QStyledItemDelegate(parent)
{
}

/* this method overrides the default table drawing method and places grid lines only at certain rows and columns */
void ProfilePrintDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	const QRect rect(option.rect);
	const int row = index.row();
	const int col = index.column();

	// grid color
	painter->setPen(QPen(QColor(0xff999999)));
	// horizontal lines
	if (row == 2 || row == 4 || row == 6)
		painter->drawLine(rect.topLeft(), rect.topRight());
	if (row == 7)
		painter->drawLine(rect.bottomLeft(), rect.bottomRight());
	// vertical lines
	if (row > 1) {
		painter->drawLine(rect.topLeft(), rect.bottomLeft());
		if (col == 4 || (col == 0 && row > 5))
			painter->drawLine(rect.topRight(), rect.bottomRight());
	}
	QStyledItemDelegate::paint(painter, option, index);
}
