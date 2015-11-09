#include "modeldelegates.h"
#include "dive.h"
#include "gettextfromc.h"
#include "mainwindow.h"
#include "cylindermodel.h"
#include "models.h"
#include "starwidget.h"
#include "profile-widget/profilewidget2.h"
#include "tankinfomodel.h"
#include "weigthsysteminfomodel.h"
#include "weightmodel.h"
#include "divetripmodel.h"
#include "qthelper.h"
#ifndef NO_MARBLE
#include "globe.h"
#endif

#include <QCompleter>
#include <QKeyEvent>
#include <QTextDocument>
#include <QApplication>
#include <QFont>
#include <QBrush>
#include <QColor>
#include <QAbstractProxyModel>

QSize DiveListDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	return QSize(50, 22);
}

// Gets the index of the model in the currentRow and column.
// currCombo is defined below.
#define IDX(_XX) mymodel->index(currCombo.currRow, (_XX))
static bool keyboardFinished = false;

StarWidgetsDelegate::StarWidgetsDelegate(QWidget *parent) : QStyledItemDelegate(parent),
	parentWidget(parent)
{
	const IconMetrics& metrics = defaultIconMetrics();
	minStarSize = QSize(metrics.sz_small * TOTALSTARS + metrics.spacing * (TOTALSTARS - 1), metrics.sz_small);
}

void StarWidgetsDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	QStyledItemDelegate::paint(painter, option, index);
	if (!index.isValid())
		return;

	QVariant value = index.model()->data(index, DiveTripModel::STAR_ROLE);
	if (!value.isValid())
		return;

	int rating = value.toInt();
	int deltaY = option.rect.height() / 2 - StarWidget::starActive().height() / 2;
	painter->save();
	painter->setRenderHint(QPainter::Antialiasing, true);
	const QPixmap active = QPixmap::fromImage(StarWidget::starActive());
	const QPixmap inactive = QPixmap::fromImage(StarWidget::starInactive());
	const IconMetrics& metrics = defaultIconMetrics();

	for (int i = 0; i < rating; i++)
		painter->drawPixmap(option.rect.x() + i * metrics.sz_small + metrics.spacing, option.rect.y() + deltaY, active);
	for (int i = rating; i < TOTALSTARS; i++)
		painter->drawPixmap(option.rect.x() + i * metrics.sz_small + metrics.spacing, option.rect.y() + deltaY, inactive);
	painter->restore();
}

QSize StarWidgetsDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	return minStarSize;
}

const QSize& StarWidgetsDelegate::starSize() const
{
	return minStarSize;
}

ComboBoxDelegate::ComboBoxDelegate(QAbstractItemModel *model, QObject *parent) : QStyledItemDelegate(parent), model(model)
{
	connect(this, SIGNAL(closeEditor(QWidget *, QAbstractItemDelegate::EndEditHint)),
		this, SLOT(revertModelData(QWidget *, QAbstractItemDelegate::EndEditHint)));
	connect(this, SIGNAL(closeEditor(QWidget *, QAbstractItemDelegate::EndEditHint)),
		this, SLOT(fixTabBehavior()));
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

struct CurrSelected {
	QComboBox *comboEditor;
	int currRow;
	QString activeText;
	QAbstractItemModel *model;
	bool ignoreSelection;
} currCombo;

QWidget *ComboBoxDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	MainWindow *m = MainWindow::instance();
	QComboBox *comboDelegate = new QComboBox(parent);
	comboDelegate->setModel(model);
	comboDelegate->setEditable(true);
	comboDelegate->setAutoCompletion(true);
	comboDelegate->setAutoCompletionCaseSensitivity(Qt::CaseInsensitive);
	comboDelegate->completer()->setCompletionMode(QCompleter::PopupCompletion);
	comboDelegate->view()->setEditTriggers(QAbstractItemView::AllEditTriggers);
	comboDelegate->lineEdit()->installEventFilter(const_cast<QObject *>(qobject_cast<const QObject *>(this)));
	if ((m->graphics()->currentState != ProfileWidget2::PROFILE))
		comboDelegate->lineEdit()->setEnabled(false);
	comboDelegate->view()->installEventFilter(const_cast<QObject *>(qobject_cast<const QObject *>(this)));
	QAbstractItemView *comboPopup = comboDelegate->lineEdit()->completer()->popup();
	comboPopup->setMouseTracking(true);
	connect(comboDelegate, SIGNAL(highlighted(QString)), this, SLOT(testActivation(QString)));
	connect(comboDelegate, SIGNAL(activated(QString)), this, SLOT(fakeActivation()));
	connect(comboPopup, SIGNAL(entered(QModelIndex)), this, SLOT(testActivation(QModelIndex)));
	connect(comboPopup, SIGNAL(activated(QModelIndex)), this, SLOT(fakeActivation()));
	currCombo.comboEditor = comboDelegate;
	currCombo.currRow = index.row();
	currCombo.model = const_cast<QAbstractItemModel *>(index.model());
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
void ComboBoxDelegate::testActivation(const QString &currText)
{
	currCombo.activeText = currText.isEmpty() ? currCombo.comboEditor->currentText() : currText;
	setModelData(currCombo.comboEditor, currCombo.model, QModelIndex());
}

void ComboBoxDelegate::testActivation(const QModelIndex &currIndex)
{
	testActivation(currIndex.data().toString());
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

// This 'reverts' the model data to what we actually choosed,
// becaus e a TAB is being understood by Qt as 'cancel' while
// we are on a QComboBox ( but not on a QLineEdit.
void ComboBoxDelegate::fixTabBehavior()
{
	if (keyboardFinished) {
		setModelData(0, 0, QModelIndex());
	}
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
			if (ev->key() == Qt::Key_Tab || ev->key() == Qt::Key_Enter || ev->key() == Qt::Key_Return) {
				currCombo.activeText = currCombo.comboEditor->currentText();
				keyboardFinished = true;
			}
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

void ComboBoxDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	QRect defaultRect = option.rect;
	defaultRect.setX(defaultRect.x() - 1);
	defaultRect.setY(defaultRect.y() - 1);
	defaultRect.setWidth(defaultRect.width() + 2);
	defaultRect.setHeight(defaultRect.height() + 2);
	editor->setGeometry(defaultRect);
}

struct RevertCylinderData {
	QString type;
	int pressure;
	int size;
} currCylinderData;

void TankInfoDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &thisindex) const
{
	CylindersModel *mymodel = qobject_cast<CylindersModel *>(currCombo.model);
	TankInfoModel *tanks = TankInfoModel::instance();
	QModelIndexList matches = tanks->match(tanks->index(0, 0), Qt::DisplayRole, currCombo.activeText);
	int row;
	QString cylinderName = currCombo.activeText;
	if (matches.isEmpty()) {
		tanks->insertRows(tanks->rowCount(), 1);
		tanks->setData(tanks->index(tanks->rowCount() - 1, 0), currCombo.activeText);
		row = tanks->rowCount() - 1;
	} else {
		row = matches.first().row();
		cylinderName = matches.first().data().toString();
	}
	int tankSize = tanks->data(tanks->index(row, TankInfoModel::ML)).toInt();
	int tankPressure = tanks->data(tanks->index(row, TankInfoModel::BAR)).toInt();

	mymodel->setData(IDX(CylindersModel::TYPE), cylinderName, Qt::EditRole);
	mymodel->passInData(IDX(CylindersModel::WORKINGPRESS), tankPressure);
	mymodel->passInData(IDX(CylindersModel::SIZE), tankSize);
}

TankInfoDelegate::TankInfoDelegate(QObject *parent) : ComboBoxDelegate(TankInfoModel::instance(), parent)
{
	connect(this, SIGNAL(closeEditor(QWidget *, QAbstractItemDelegate::EndEditHint)),
		this, SLOT(reenableReplot(QWidget *, QAbstractItemDelegate::EndEditHint)));
}

void TankInfoDelegate::reenableReplot(QWidget *widget, QAbstractItemDelegate::EndEditHint hint)
{
	MainWindow::instance()->graphics()->setReplot(true);
	// FIXME: We need to replot after a cylinder is selected but the replot below overwrites
	//        the newly selected cylinder.
	//	MainWindow::instance()->graphics()->replot();
}

void TankInfoDelegate::revertModelData(QWidget *widget, QAbstractItemDelegate::EndEditHint hint)
{
	if (hint == QAbstractItemDelegate::NoHint ||
	    hint == QAbstractItemDelegate::RevertModelCache) {
		CylindersModel *mymodel = qobject_cast<CylindersModel *>(currCombo.model);
		mymodel->setData(IDX(CylindersModel::TYPE), currCylinderData.type, Qt::EditRole);
		mymodel->passInData(IDX(CylindersModel::WORKINGPRESS), currCylinderData.pressure);
		mymodel->passInData(IDX(CylindersModel::SIZE), currCylinderData.size);
	}
}

QWidget *TankInfoDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	// ncreate editor needs to be called before because it will populate a few
	// things in the currCombo global var.
	QWidget *delegate = ComboBoxDelegate::createEditor(parent, option, index);
	CylindersModel *mymodel = qobject_cast<CylindersModel *>(currCombo.model);
	cylinder_t *cyl = mymodel->cylinderAt(index);
	currCylinderData.type = copy_string(cyl->type.description);
	currCylinderData.pressure = cyl->type.workingpressure.mbar;
	currCylinderData.size = cyl->type.size.mliter;
	MainWindow::instance()->graphics()->setReplot(false);
	return delegate;
}

TankUseDelegate::TankUseDelegate(QObject *parent) : QStyledItemDelegate(parent)
{
}

QWidget *TankUseDelegate::createEditor(QWidget * parent, const QStyleOptionViewItem & option, const QModelIndex & index) const
{
	QComboBox *comboBox = new QComboBox(parent);
	for (int i = 0; i < NUM_GAS_USE; i++)
		comboBox->addItem(gettextFromC::instance()->trGettext(cylinderuse_text[i]));
	return comboBox;
}

void TankUseDelegate::setEditorData(QWidget * editor, const QModelIndex & index) const
{
	QComboBox *comboBox = qobject_cast<QComboBox*>(editor);
	QString indexString = index.data().toString();
	comboBox->setCurrentIndex(cylinderuse_from_text(indexString.toUtf8().data()));
}

void TankUseDelegate::setModelData(QWidget * editor, QAbstractItemModel * model, const QModelIndex & index) const
{
	QComboBox *comboBox = qobject_cast<QComboBox*>(editor);
	model->setData(index, comboBox->currentIndex());
}

struct RevertWeightData {
	QString type;
	int weight;
} currWeight;

void WSInfoDelegate::revertModelData(QWidget *widget, QAbstractItemDelegate::EndEditHint hint)
{
	if (hint == QAbstractItemDelegate::NoHint ||
	    hint == QAbstractItemDelegate::RevertModelCache) {
		WeightModel *mymodel = qobject_cast<WeightModel *>(currCombo.model);
		mymodel->setData(IDX(WeightModel::TYPE), currWeight.type, Qt::EditRole);
		mymodel->passInData(IDX(WeightModel::WEIGHT), currWeight.weight);
	}
}

void WSInfoDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &thisindex) const
{
	WeightModel *mymodel = qobject_cast<WeightModel *>(currCombo.model);
	WSInfoModel *wsim = WSInfoModel::instance();
	QModelIndexList matches = wsim->match(wsim->index(0, 0), Qt::DisplayRole, currCombo.activeText);
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

WSInfoDelegate::WSInfoDelegate(QObject *parent) : ComboBoxDelegate(WSInfoModel::instance(), parent)
{
}

QWidget *WSInfoDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	/* First, call the combobox-create editor, it will setup our globals. */
	QWidget *editor = ComboBoxDelegate::createEditor(parent, option, index);
	WeightModel *mymodel = qobject_cast<WeightModel *>(currCombo.model);
	weightsystem_t *ws = mymodel->weightSystemAt(index);
	currWeight.type = copy_string(ws->description);
	currWeight.weight = ws->weight.grams;
	return editor;
}

void AirTypesDelegate::revertModelData(QWidget *widget, QAbstractItemDelegate::EndEditHint hint)
{
}

void AirTypesDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
	if (!index.isValid())
		return;
	QComboBox *combo = qobject_cast<QComboBox *>(editor);
	model->setData(index, QVariant(combo->currentText()));
}

AirTypesDelegate::AirTypesDelegate(QObject *parent) : ComboBoxDelegate(GasSelectionModel::instance(), parent)
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

LocationFilterDelegate::LocationFilterDelegate(QObject *parent)
{
}

void LocationFilterDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &origIdx) const
{
	QFont fontBigger = qApp->font();
	QFont fontSmaller = qApp->font();
	QFontMetrics fmBigger(fontBigger);
	QStyleOptionViewItemV4 opt = option;
	const QAbstractProxyModel *proxyModel = dynamic_cast<const QAbstractProxyModel*>(origIdx.model());
	QModelIndex index = proxyModel->mapToSource(origIdx);
	QStyledItemDelegate::initStyleOption(&opt, index);
	QString diveSiteName = index.data().toString();
	QString bottomText;
	QIcon icon = index.data(Qt::DecorationRole).value<QIcon>();
	struct dive_site *ds = get_dive_site_by_uuid(
		index.model()->data(index.model()->index(index.row(),0)).toInt()
	);
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
		int idx = taxonomy_index_for_category(&ds->taxonomy, prefs.geocoding.category[i]);
		if (idx == -1)
			continue;
		if(!bottomText.isEmpty())
			bottomText += " / ";
		bottomText += QString(ds->taxonomy.category[idx].value);
	}

	if (bottomText.isEmpty()) {
		const char *gpsCoords = printGPSCoords(ds->latitude.udeg, ds->longitude.udeg);
		bottomText = QString(gpsCoords);
		free( (void*) gpsCoords);
	}

	if (dive_site_has_gps_location(ds) && dive_site_has_gps_location(&displayed_dive_site)) {
		// so we are showing a completion and both the current dive site and the completion
		// have a GPS fix... so let's show the distance
		if (ds->latitude.udeg == displayed_dive_site.latitude.udeg &&
		    ds->longitude.udeg == displayed_dive_site.longitude.udeg) {
			bottomText += tr(" (same GPS fix)");
		} else {
			int distanceMeters = get_distance(ds->latitude, ds->longitude, displayed_dive_site.latitude, displayed_dive_site.longitude);
			QString distance = distance_string(distanceMeters);
			int nr = nr_of_dives_at_dive_site(ds->uuid, false);
			bottomText += tr(" (~%1 away").arg(distance);
			bottomText += tr(", %n dive(s) here)", "", nr);
		}
	}
	if (bottomText.isEmpty()) {
		if (dive_site_has_gps_location(&displayed_dive_site))
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
		const qreal pad = 1.0;
		const qreal pad2 = pad * 2.0;
		const qreal rounding = 5.0;
		painter->drawRoundedRect(option.rect.x() + pad,
			option.rect.y() + pad,
			option.rect.width() - pad2,
			option.rect.height() - pad2,
			rounding, rounding);
	}
	painter->setPen(textPen);
	painter->setFont(fontBigger);
	const qreal textPad = 5.0;
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
