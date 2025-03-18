// SPDX-License-Identifier: GPL-2.0

#include "desktop-widgets/modeldelegates.h"
#include "core/sample.h"
#include "core/subsurface-string.h"
#include "core/gettextfromc.h"
#include "desktop-widgets/mainwindow.h"
#include "qt-models/cylindermodel.h"
#include "qt-models/models.h"
#include "desktop-widgets/starwidget.h"
#include "profile-widget/profilewidget2.h"
#include "qt-models/tankinfomodel.h"
#include "qt-models/weightsysteminfomodel.h"
#include "qt-models/weightmodel.h"
#include "qt-models/diveplannermodel.h"
#include "qt-models/divetripmodel.h"
#include "qt-models/divelocationmodel.h"
#include "core/qthelper.h"
#include "core/divesite.h"
#include "core/selection.h"
#include "desktop-widgets/simplewidgets.h"

#include <QCompleter>
#include <QKeyEvent>
#include <QTextDocument>
#include <QApplication>
#include <QFont>
#include <QBrush>
#include <QColor>
#include <QAbstractProxyModel>
#include <QLineEdit>
#include <QAbstractItemView>
#include <QSpinBox>

QSize DiveListDelegate::sizeHint(const QStyleOptionViewItem &, const QModelIndex &) const
{
	const QFontMetrics metrics(qApp->font());
	return QSize(50, std::max(22, metrics.height()));
}

// Gets the index of the model in the currentRow and column.
// currCombo is defined below.
#define IDX(_XX) mymodel->index(currCombo.currRow, (_XX))

StarWidgetsDelegate::StarWidgetsDelegate(QWidget *parent) : QStyledItemDelegate(parent),
	parentWidget(parent)
{
	const IconMetrics &metrics = defaultIconMetrics();
	minStarSize = QSize(metrics.sz_small * TOTALSTARS + metrics.spacing * (TOTALSTARS - 1), metrics.sz_small);
}

void StarWidgetsDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	QStyledItemDelegate::paint(painter, option, index);
	if (!index.isValid())
		return;

	QVariant value = index.model()->data(index, DiveTripModelBase::STAR_ROLE);
	if (!value.isValid())
		return;

	int rating = value.toInt();
	int deltaY = option.rect.height() / 2 - StarWidget::starActive().height() / 2;
	painter->save();
	painter->setRenderHint(QPainter::Antialiasing, true);
	const QPixmap active = QPixmap::fromImage(StarWidget::starActive());
	const QPixmap inactive = QPixmap::fromImage(StarWidget::starInactive());
	const IconMetrics &metrics = defaultIconMetrics();

	for (int i = 0; i < rating; i++)
		painter->drawPixmap(option.rect.x() + i * metrics.sz_small + metrics.spacing, option.rect.y() + deltaY, active);
	for (int i = rating; i < TOTALSTARS; i++)
		painter->drawPixmap(option.rect.x() + i * metrics.sz_small + metrics.spacing, option.rect.y() + deltaY, inactive);
	painter->restore();
}

QSize StarWidgetsDelegate::sizeHint(const QStyleOptionViewItem &, const QModelIndex &) const
{
	return minStarSize;
}

const QSize &StarWidgetsDelegate::starSize() const
{
	return minStarSize;
}

ComboBoxDelegate::ComboBoxDelegate(std::function<QAbstractItemModel *(QWidget *)> create_model_func,
				   QObject *parent, bool allowEdit) : QStyledItemDelegate(parent),
	create_model_func(std::move(create_model_func)),
	editable(allowEdit)
{
	connect(this, &ComboBoxDelegate::closeEditor, this, &ComboBoxDelegate::editorClosed);
}

void ComboBoxDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
	QComboBox *c = qobject_cast<QComboBox *>(editor);
	QString data = index.model()->data(index, Qt::DisplayRole).toString();
	int i = c->findText(data);
	if (i != -1)
		c->setCurrentIndex(i);
	else
		c->setEditText(data);
	c->lineEdit()->setSelection(0, c->lineEdit()->text().length());
}

QWidget *ComboBoxDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &, const QModelIndex &index) const
{
	QComboBox *comboDelegate = new QComboBox(parent);
	comboDelegate->setModel(create_model_func(comboDelegate));
	comboDelegate->setEditable(true);
	comboDelegate->completer()->setCaseSensitivity(Qt::CaseInsensitive);
	comboDelegate->completer()->setCompletionMode(QCompleter::PopupCompletion);
	comboDelegate->completer()->setFilterMode(Qt::MatchContains);
	comboDelegate->view()->setEditTriggers(QAbstractItemView::AllEditTriggers);
	comboDelegate->lineEdit()->installEventFilter(const_cast<QObject *>(qobject_cast<const QObject *>(this)));
	comboDelegate->lineEdit()->setEnabled(editable);
	comboDelegate->view()->installEventFilter(const_cast<QObject *>(qobject_cast<const QObject *>(this)));
	QAbstractItemView *comboPopup = comboDelegate->lineEdit()->completer()->popup();
	comboPopup->setMouseTracking(true);
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
	connect(comboDelegate, &QComboBox::textHighlighted, this, &ComboBoxDelegate::testActivationString);
#else
	connect(comboDelegate, QOverload<const QString &>::of(&QComboBox::highlighted), this, &ComboBoxDelegate::testActivationString);
#endif
	connect(comboDelegate, QOverload<int>::of(&QComboBox::activated), this, &ComboBoxDelegate::fakeActivation);
	connect(comboPopup, &QAbstractItemView::entered, this, &ComboBoxDelegate::testActivationIndex);
	connect(comboPopup, &QAbstractItemView::activated, this, &ComboBoxDelegate::fakeActivation);
	currCombo.comboEditor = comboDelegate;
	currCombo.currRow = index.row();
	currCombo.model = const_cast<QAbstractItemModel *>(index.model());
	currCombo.activeText = currCombo.model->data(index).toString();
	currCombo.ignoreSelection = false;

	return comboDelegate;
}

/* This Method is being called when the user *writes* something and press enter or tab,
 * and it`s also called when the mouse walks over the list of choices from the ComboBox,
 * One thing is important, if the user writes a *new* cylinder or weight type, it will
 * be ADDED to the list, and the user will need to fill the other data.
 */
void ComboBoxDelegate::testActivationString(const QString &currText)
{
	currCombo.activeText = currText.isEmpty() ? currCombo.comboEditor->currentText() : currText;
	setModelData(currCombo.comboEditor, currCombo.model, QModelIndex());
}

void ComboBoxDelegate::testActivationIndex(const QModelIndex &currIndex)
{
	testActivationString(currIndex.data().toString());
}

// HACK, send a fake event so Qt thinks we hit 'enter' on the line edit.
void ComboBoxDelegate::fakeActivation()
{
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

bool ComboBoxDelegate::eventFilter(QObject *object, QEvent *event)
{
	// Reacts on Key_UP and Key_DOWN to show the QComboBox - list of choices.
	if (event->type() == QEvent::KeyPress || event->type() == QEvent::ShortcutOverride) {
		if (object == currCombo.comboEditor) { // the 'LineEdit' part
			QKeyEvent *ev = static_cast<QKeyEvent *>(event);
			if (ev->key() == Qt::Key_Up || ev->key() == Qt::Key_Down) {
				currCombo.ignoreSelection = true;
				if (!currCombo.comboEditor->completer()->popup()->isVisible()) {
					currCombo.comboEditor->showPopup();
					return true;
				}
			}
			if (ev->key() == Qt::Key_Tab || ev->key() == Qt::Key_Enter || ev->key() == Qt::Key_Return)
				currCombo.activeText = currCombo.comboEditor->currentText();
		} else { // the 'Drop Down Menu' part.
			QKeyEvent *ev = static_cast<QKeyEvent *>(event);
			if (ev->key() == Qt::Key_Enter || ev->key() == Qt::Key_Return ||
			    ev->key() == Qt::Key_Tab || ev->key() == Qt::Key_Backtab ||
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

void ComboBoxDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &) const
{
	QRect defaultRect = option.rect;
	defaultRect.setX(defaultRect.x() - 1);
	defaultRect.setY(defaultRect.y() - 1);
	defaultRect.setWidth(defaultRect.width() + 2);
	defaultRect.setHeight(defaultRect.height() + 2);
	editor->setGeometry(defaultRect);
}

void TankInfoDelegate::setModelData(QWidget *, QAbstractItemModel *model, const QModelIndex &index) const
{
	QAbstractItemModel *mymodel = currCombo.model;
	QString cylinderName = currCombo.activeText.trimmed();
	if (cylinderName.isEmpty()) {
		mymodel->setData(IDX(CylindersModel::TYPE), cylinderName, CylindersModel::TEMP_ROLE);
		return;
	}

	auto [tankSize, tankPressure] = get_tank_info_data(tank_info_table, cylinderName.toStdString());
	mymodel->setData(IDX(CylindersModel::TYPE), cylinderName, CylindersModel::TEMP_ROLE);
	mymodel->setData(IDX(CylindersModel::WORKINGPRESS), tankPressure.mbar, CylindersModel::TEMP_ROLE);
	mymodel->setData(IDX(CylindersModel::SIZE), tankSize.mliter, CylindersModel::TEMP_ROLE);
}

static QAbstractItemModel *createTankInfoModel(QWidget *parent)
{
	return new TankInfoModel(parent);
}

TankInfoDelegate::TankInfoDelegate(QObject *parent) : ComboBoxDelegate(&createTankInfoModel, parent, true)
{
}

void TankInfoDelegate::editorClosed(QWidget *, QAbstractItemDelegate::EndEditHint hint)
{
	QAbstractItemModel *mymodel = currCombo.model;
	// Ugly hack: We misuse setData() with COMMIT_ROLE or REVERT_ROLE to commit or
	// revert the current row. We send in the type, because we may get multiple
	// end events and thus can prevent multiple commits.
	if (hint == QAbstractItemDelegate::RevertModelCache)
		mymodel->setData(IDX(CylindersModel::TYPE), currCombo.activeText, CylindersModel::REVERT_ROLE);
	else
		mymodel->setData(IDX(CylindersModel::TYPE), currCombo.activeText, CylindersModel::COMMIT_ROLE);
}

TankUseDelegate::TankUseDelegate(QObject *parent) : QStyledItemDelegate(parent), currentDive(nullptr), currentDcNr(0)
{
}

void TankUseDelegate::setDiveDc(const dive &d, int &dcNr)
{
	currentDive = &d;
	currentDcNr = &dcNr;
}

QWidget *TankUseDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &, const QModelIndex &) const
{
	QComboBox *comboBox = new QComboBox(parent);
	const divecomputer *dc = currentDive->get_dc(*currentDcNr);
	if (!dc)
		return comboBox;
	bool isCcrDive = dc->divemode == CCR;
	bool isFreeDive = dc->divemode == FREEDIVE;
	for (int i = 0; i < NUM_GAS_USE; i++) {
		if (isFreeDive && i != NOT_USED)
			continue;

		if (!isCcrDive && (i == DILUENT || i == OXYGEN))
			continue;

		comboBox->addItem(gettextFromC::tr(cylinderuse_text[i]));
	}
	return comboBox;
}

void TankUseDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
	QComboBox *comboBox = qobject_cast<QComboBox *>(editor);
	QString indexString = index.data().toString();
	comboBox->setCurrentIndex(cylinderuse_from_text(qPrintable(indexString)));
}

void TankUseDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
	QComboBox *comboBox = qobject_cast<QComboBox *>(editor);
	model->setData(index, cylinderuse_from_text(qPrintable(comboBox->currentText())));
}

SensorDelegate::SensorDelegate(QObject *parent) :
	ComboBoxDelegate([this] (QWidget *parent) { return new SensorSelectionModel(*this->getCurrentDc(), parent); }, parent, false), currentdc(nullptr)
{
}

void SensorDelegate::setCurrentDc(divecomputer *dc)
{
	currentdc = dc;
}

const divecomputer *SensorDelegate::getCurrentDc() const
{
	return currentdc;
}

void SensorDelegate::editorClosed(QWidget *, QAbstractItemDelegate::EndEditHint)
{
}

void SensorDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
	if (!index.isValid())
		return;
	QComboBox *combo = qobject_cast<QComboBox *>(editor);
	model->setData(index, combo->currentData(Qt::UserRole));
}

void WSInfoDelegate::editorClosed(QWidget *, QAbstractItemDelegate::EndEditHint hint)
{
	WeightModel *mymodel = qobject_cast<WeightModel *>(currCombo.model);
	if (hint == QAbstractItemDelegate::RevertModelCache)
		mymodel->clearTempWS();
	else
		mymodel->commitTempWS();
}

void WSInfoDelegate::setModelData(QWidget *, QAbstractItemModel *, const QModelIndex &) const
{
	WeightModel *mymodel = qobject_cast<WeightModel *>(currCombo.model);
	std::string weightName = currCombo.activeText.toStdString();
	weight_t weight = get_weightsystem_weight(weightName.c_str());

	mymodel->setTempWS(currCombo.currRow, weightsystem_t( weight, std::move(weightName), false ));
}

static QAbstractItemModel *createWSInfoModel(QWidget *parent)
{
	return new WSInfoModel(parent);
}

WSInfoDelegate::WSInfoDelegate(QObject *parent) : ComboBoxDelegate(&createWSInfoModel, parent, true)
{
}

void GasTypesDelegate::editorClosed(QWidget *, QAbstractItemDelegate::EndEditHint)
{
}

void GasTypesDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
	if (!index.isValid())
		return;
	QComboBox *combo = qobject_cast<QComboBox *>(editor);
	model->setData(index, combo->currentData(Qt::UserRole));
}

GasTypesDelegate::GasTypesDelegate(const dive &d, int &dcNr, QObject *parent) :
	ComboBoxDelegate([&d, &dcNr] (QWidget *parent) { return new GasSelectionModel(d, dcNr, parent); }, parent, false)
{
}

void DiveTypesDelegate::editorClosed(QWidget *, QAbstractItemDelegate::EndEditHint)
{
}

void DiveTypesDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
	if (!index.isValid())
		return;
	QComboBox *combo = qobject_cast<QComboBox *>(editor);
	model->setData(index, combo->currentData(Qt::UserRole));
}

DiveTypesDelegate::DiveTypesDelegate(const dive &d, int &dcNr, QObject *parent) :
	ComboBoxDelegate([&d, &dcNr] (QWidget *parent) { return new DiveTypeSelectionModel(d, dcNr, parent); }, parent, false)
{
}

SpinBoxDelegate::SpinBoxDelegate(int min, int max, int step, QObject *parent):
	QStyledItemDelegate(parent),
	min(min),
	max(max),
	step(step)
{
}

QWidget *SpinBoxDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	QSpinBox *w = qobject_cast<QSpinBox*>(QStyledItemDelegate::createEditor(parent, option, index));
	w->setRange(min,max);
	w->setSingleStep(step);
	return w;
}

DoubleSpinBoxDelegate::DoubleSpinBoxDelegate(double min, double max, double step, QObject *parent):
	QStyledItemDelegate(parent),
	min(min),
	max(max),
	step(step)
{
}

QWidget *DoubleSpinBoxDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	QDoubleSpinBox *w = qobject_cast<QDoubleSpinBox*>(QStyledItemDelegate::createEditor(parent, option, index));
	w->setRange(min,max);
	w->setSingleStep(step);
	return w;
}

LocationFilterDelegate::LocationFilterDelegate(QObject *)
{
}

void LocationFilterDelegate::setCurrentLocation(location_t loc)
{
	currentLocation = loc;
}

void LocationFilterDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &origIdx) const
{
	QFont fontBigger = qApp->font();
	QFont fontSmaller = qApp->font();
	QFontMetrics fmBigger(fontBigger);
	QStyleOptionViewItem opt = option;
	const QAbstractProxyModel *proxyModel = dynamic_cast<const QAbstractProxyModel *>(origIdx.model());
	if (!proxyModel)
		return;
	QModelIndex index = proxyModel->mapToSource(origIdx);
	QStyledItemDelegate::initStyleOption(&opt, index);
	QString diveSiteName = index.data().toString();
	QString bottomText;
	QIcon icon = index.data(Qt::DecorationRole).value<QIcon>();
	struct dive_site *ds =
		index.model()->data(index.model()->index(index.row(), LocationInformationModel::DIVESITE)).value<dive_site *>();
	bool currentDiveHasGPS = has_location(&currentLocation);
	//Special case: do not show name, but instead, show
	if (index.row() < 2) {
		diveSiteName = index.data().toString();
		bottomText = index.data(Qt::ToolTipRole).toString();
		goto print_part;
	}

	if (!ds)
		return;

	for (int i = 0; i < 3; i++) {
		if (prefs.geocoding.category[i] == TC_NONE)
			continue;
		std::string value = taxonomy_get_value(ds->taxonomy, prefs.geocoding.category[i]);
		if (!value.empty())
			continue;
		if(!bottomText.isEmpty())
			bottomText += " / ";
		bottomText += QString::fromStdString(value);
	}

	if (bottomText.isEmpty())
		bottomText = printGPSCoords(&ds->location);

	if (ds->has_gps_location() && currentDiveHasGPS) {
		// so we are showing a completion and both the current dive site and the completion
		// have a GPS fix... so let's show the distance
		if (ds->location == currentLocation) {
			bottomText += tr(" (same GPS fix)");
		} else {
			int distanceMeters = get_distance(ds->location, currentLocation);
			QString distance = distance_string(distanceMeters);
			size_t nr = ds->nr_of_dives();
			bottomText += tr(" (~%1 away").arg(distance);
			bottomText += tr(", %n dive(s) here)", "", nr);
		}
	}
	if (bottomText.isEmpty()) {
		if (currentDiveHasGPS)
			bottomText = tr("(no existing GPS data, add GPS fix from this dive)");
		else
			bottomText = tr("(no GPS data)");
	}
	bottomText = tr("Pick site: ") + bottomText;

print_part:

	fontBigger.setPointSize(fontBigger.pointSize() + 1);
	fontBigger.setBold(true);
	QPen textPen = QPen(option.state & QStyle::State_Selected ? option.palette.highlightedText().color() : option.palette.text().color(), 1);

	initStyleOption(&opt, index);
	opt.text = QString();
	opt.icon = QIcon();
	painter->setClipRect(option.rect);

	painter->save();
	if (option.state & QStyle::State_Selected) {
		painter->setPen(QPen(opt.palette.highlight().color().darker()));
		painter->setBrush(opt.palette.highlight());
		const int pad = 1;
		const int pad2 = pad * 2;
		const int rounding = 5;
		painter->drawRoundedRect(option.rect.x() + pad,
					option.rect.y() + pad,
					option.rect.width() - pad2,
					option.rect.height() - pad2,
					rounding, rounding);
	}
	painter->setPen(textPen);
	painter->setFont(fontBigger);
	const int textPad = 5;
	painter->drawText(option.rect.x() + textPad, option.rect.y() + fmBigger.boundingRect("YH").height(), diveSiteName);
	double pointSize = fontSmaller.pointSizeF();
	fontSmaller.setPointSizeF(0.9 * pointSize);
	painter->setFont(fontSmaller);
	painter->drawText(option.rect.x() + textPad, option.rect.y() + fmBigger.boundingRect("YH").height() * 2, bottomText);
	painter->restore();

	if (!icon.isNull()) {
		painter->save();
		painter->drawPixmap(
			option.rect.x() + option.rect.width() - 24,
			option.rect.y() + option.rect.height() - 24, icon.pixmap(20,20));
		painter->restore();
	}
}

QSize LocationFilterDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	QFont fontBigger = qApp->font();
	fontBigger.setPointSize(fontBigger.pointSize());
	fontBigger.setBold(true);

	QFontMetrics fmBigger(fontBigger);

	QFont fontSmaller = qApp->font();
	QFontMetrics fmSmaller(fontSmaller);

	QSize retSize = QStyledItemDelegate::sizeHint(option, index);
	retSize.setHeight(
		fmBigger.boundingRect("Yellow House").height() + 5 /*spacing*/ +
		fmSmaller.boundingRect("Yellow House").height());

	return retSize;
}
