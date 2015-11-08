#include "locationinformation.h"
#include "dive.h"
#include "mainwindow.h"
#include "divelistview.h"
#include "qthelper.h"
#include "globe.h"
#include "filtermodels.h"
#include "divelocationmodel.h"
#include "divesitehelpers.h"
#include "modeldelegates.h"

#include <QDebug>
#include <QShowEvent>
#include <QItemSelectionModel>
#include <qmessagebox.h>
#include <cstdlib>
#include <QDesktopWidget>
#include <QScrollBar>

LocationInformationWidget::LocationInformationWidget(QWidget *parent) : QGroupBox(parent), modified(false)
{
	ui.setupUi(this);
	ui.diveSiteMessage->setCloseButtonVisible(false);

	acceptAction = new QAction(tr("Apply changes"), this);
	connect(acceptAction, SIGNAL(triggered(bool)), this, SLOT(acceptChanges()));

	rejectAction = new QAction(tr("Discard changes"), this);
	connect(rejectAction, SIGNAL(triggered(bool)), this, SLOT(rejectChanges()));

	ui.diveSiteMessage->setText(tr("Dive site management"));
	ui.diveSiteMessage->addAction(acceptAction);
	ui.diveSiteMessage->addAction(rejectAction);

	connect(this, SIGNAL(startFilterDiveSite(uint32_t)), MultiFilterSortModel::instance(), SLOT(startFilterDiveSite(uint32_t)));
	connect(this, SIGNAL(stopFilterDiveSite()), MultiFilterSortModel::instance(), SLOT(stopFilterDiveSite()));
	connect(ui.geoCodeButton, SIGNAL(clicked()), this, SLOT(reverseGeocode()));

	SsrfSortFilterProxyModel *filter_model = new SsrfSortFilterProxyModel(this);
	filter_model->setSourceModel(LocationInformationModel::instance());
	filter_model->setFilterRow(filter_same_gps_cb);
	ui.diveSiteListView->setModel(filter_model);
	ui.diveSiteListView->setModelColumn(LocationInformationModel::NAME);
	ui.diveSiteListView->installEventFilter(this);
#ifndef NO_MARBLE
	// Globe Management Code.
	connect(this, &LocationInformationWidget::requestCoordinates,
		GlobeGPS::instance(), &GlobeGPS::prepareForGetDiveCoordinates);
	connect(this, &LocationInformationWidget::endRequestCoordinates,
		GlobeGPS::instance(), &GlobeGPS::endGetDiveCoordinates);
	connect(GlobeGPS::instance(), &GlobeGPS::coordinatesChanged,
		this, &LocationInformationWidget::updateGpsCoordinates);
	connect(this, &LocationInformationWidget::endEditDiveSite,
		GlobeGPS::instance(), &GlobeGPS::repopulateLabels);
#endif
}

bool LocationInformationWidget::eventFilter(QObject *, QEvent *ev)
{
	if (ev->type() == QEvent::ContextMenu) {
		QContextMenuEvent *ctx = (QContextMenuEvent *)ev;
		QMenu contextMenu;
		contextMenu.addAction(tr("Merge into current site"), this, SLOT(mergeSelectedDiveSites()));
		contextMenu.exec(ctx->globalPos());
		return true;
	}
	return false;
}

void LocationInformationWidget::mergeSelectedDiveSites()
{
	if (QMessageBox::warning(MainWindow::instance(), tr("Merging dive sites"),
				 tr("You are about to merge dive sites, you can't undo that action \n Are you sure you want to continue?"),
				 QMessageBox::Ok, QMessageBox::Cancel) != QMessageBox::Ok)
		return;

	QModelIndexList selection = ui.diveSiteListView->selectionModel()->selectedIndexes();
	uint32_t *selected_dive_sites = (uint32_t *)malloc(sizeof(uint32_t) * selection.count());
	int i = 0;
	Q_FOREACH (const QModelIndex &idx, selection) {
		selected_dive_sites[i] = (uint32_t)idx.data(LocationInformationModel::UUID_ROLE).toInt();
		i++;
	}
	merge_dive_sites(displayed_dive_site.uuid, selected_dive_sites, i);
	LocationInformationModel::instance()->update();
	QSortFilterProxyModel *m = (QSortFilterProxyModel *)ui.diveSiteListView->model();
	m->invalidate();
	free(selected_dive_sites);
}

void LocationInformationWidget::updateLabels()
{
	if (displayed_dive_site.name)
		ui.diveSiteName->setText(displayed_dive_site.name);
	else
		ui.diveSiteName->clear();
	if (displayed_dive_site.description)
		ui.diveSiteDescription->setText(displayed_dive_site.description);
	else
		ui.diveSiteDescription->clear();
	if (displayed_dive_site.notes)
		ui.diveSiteNotes->setPlainText(displayed_dive_site.notes);
	else
		ui.diveSiteNotes->clear();
	if (displayed_dive_site.latitude.udeg || displayed_dive_site.longitude.udeg) {
		const char *coords = printGPSCoords(displayed_dive_site.latitude.udeg, displayed_dive_site.longitude.udeg);
		ui.diveSiteCoordinates->setText(coords);
		free((void *)coords);
	} else {
		ui.diveSiteCoordinates->clear();
	}

	ui.locationTags->setText(constructLocationTags(displayed_dive_site.uuid));

	emit startFilterDiveSite(displayed_dive_site.uuid);
	emit startEditDiveSite(displayed_dive_site.uuid);
}

void LocationInformationWidget::updateGpsCoordinates()
{
	QString oldText = ui.diveSiteCoordinates->text();
	const char *coords = printGPSCoords(displayed_dive_site.latitude.udeg, displayed_dive_site.longitude.udeg);
	ui.diveSiteCoordinates->setText(coords);
	free((void *)coords);
	if (oldText != ui.diveSiteCoordinates->text())
		markChangedWidget(ui.diveSiteCoordinates);
}

void LocationInformationWidget::acceptChanges()
{
	char *uiString;
	struct dive_site *currentDs;
	uiString = ui.diveSiteName->text().toUtf8().data();

	if (get_dive_site_by_uuid(displayed_dive_site.uuid) != NULL)
		currentDs = get_dive_site_by_uuid(displayed_dive_site.uuid);
	else
		currentDs = get_dive_site_by_uuid(create_dive_site_from_current_dive(uiString));

	currentDs->latitude = displayed_dive_site.latitude;
	currentDs->longitude = displayed_dive_site.longitude;
	if (!same_string(uiString, currentDs->name)) {
		free(currentDs->name);
		currentDs->name = copy_string(uiString);
	}
	uiString = ui.diveSiteDescription->text().toUtf8().data();
	if (!same_string(uiString, currentDs->description)) {
		free(currentDs->description);
		currentDs->description = copy_string(uiString);
	}
	uiString = ui.diveSiteNotes->document()->toPlainText().toUtf8().data();
	if (!same_string(uiString, currentDs->notes)) {
		free(currentDs->notes);
		currentDs->notes = copy_string(uiString);
	}
	if (!ui.diveSiteCoordinates->text().isEmpty()) {
		double lat, lon;
		parseGpsText(ui.diveSiteCoordinates->text(), &lat, &lon);
		currentDs->latitude.udeg = lat * 1000000.0;
		currentDs->longitude.udeg = lon * 1000000.0;
	}
	if (dive_site_is_empty(currentDs)) {
		LocationInformationModel::instance()->removeRow(get_divesite_idx(currentDs));
		displayed_dive.dive_site_uuid = 0;
	}
	copy_dive_site(currentDs, &displayed_dive_site);
	mark_divelist_changed(true);
	resetState();
	emit endRequestCoordinates();
	emit endEditDiveSite();
	emit stopFilterDiveSite();
	emit coordinatesChanged();
}

void LocationInformationWidget::rejectChanges()
{
	resetState();
	emit endRequestCoordinates();
	emit stopFilterDiveSite();
	emit endEditDiveSite();
	emit coordinatesChanged();
}

void LocationInformationWidget::showEvent(QShowEvent *ev)
{
	if (displayed_dive_site.uuid) {
		updateLabels();
		ui.geoCodeButton->setEnabled(dive_site_has_gps_location(&displayed_dive_site));
		QSortFilterProxyModel *m = qobject_cast<QSortFilterProxyModel *>(ui.diveSiteListView->model());
		emit startFilterDiveSite(displayed_dive_site.uuid);
		if (m)
			m->invalidate();
	}
	emit requestCoordinates();

	QGroupBox::showEvent(ev);
}

void LocationInformationWidget::markChangedWidget(QWidget *w)
{
	QPalette p;
	qreal h, s, l, a;
	if (!modified)
		enableEdition();
	qApp->palette().color(QPalette::Text).getHslF(&h, &s, &l, &a);
	p.setBrush(QPalette::Base, (l <= 0.3) ? QColor(Qt::yellow).lighter() : (l <= 0.6) ? QColor(Qt::yellow).light() : /* else */ QColor(Qt::yellow).darker(300));
	w->setPalette(p);
	modified = true;
}

void LocationInformationWidget::resetState()
{
	modified = false;
	resetPallete();
	MainWindow::instance()->dive_list()->setEnabled(true);
	MainWindow::instance()->setEnabledToolbar(true);
	ui.diveSiteMessage->setText(tr("Dive site management"));
}

void LocationInformationWidget::enableEdition()
{
	MainWindow::instance()->dive_list()->setEnabled(false);
	MainWindow::instance()->setEnabledToolbar(false);
	ui.diveSiteMessage->setText(tr("You are editing a dive site"));
}

void LocationInformationWidget::on_diveSiteCoordinates_textChanged(const QString &text)
{
	uint lat = displayed_dive_site.latitude.udeg;
	uint lon = displayed_dive_site.longitude.udeg;
	const char *coords = printGPSCoords(lat, lon);
	if (!same_string(qPrintable(text), coords)) {
		double latitude, longitude;
		if (parseGpsText(text, &latitude, &longitude)) {
			displayed_dive_site.latitude.udeg = latitude * 1000000;
			displayed_dive_site.longitude.udeg = longitude * 1000000;
			markChangedWidget(ui.diveSiteCoordinates);
			emit coordinatesChanged();
			ui.geoCodeButton->setEnabled(latitude != 0 && longitude != 0);
		} else {
			ui.geoCodeButton->setEnabled(false);
		}
	}
	free((void *)coords);
}

void LocationInformationWidget::on_diveSiteDescription_textChanged(const QString &text)
{
	if (!same_string(qPrintable(text), displayed_dive_site.description))
		markChangedWidget(ui.diveSiteDescription);
}

void LocationInformationWidget::on_diveSiteName_textChanged(const QString &text)
{
	if (!same_string(qPrintable(text), displayed_dive_site.name))
		markChangedWidget(ui.diveSiteName);
}

void LocationInformationWidget::on_diveSiteNotes_textChanged()
{
	if (!same_string(qPrintable(ui.diveSiteNotes->toPlainText()), displayed_dive_site.notes))
		markChangedWidget(ui.diveSiteNotes);
}

void LocationInformationWidget::resetPallete()
{
	QPalette p;
	ui.diveSiteCoordinates->setPalette(p);
	ui.diveSiteDescription->setPalette(p);
	ui.diveSiteName->setPalette(p);
	ui.diveSiteNotes->setPalette(p);
}

void LocationInformationWidget::reverseGeocode()
{
	ReverseGeoLookupThread *geoLookup = ReverseGeoLookupThread::instance();
	geoLookup->lookup(&displayed_dive_site);
	updateLabels();
}

DiveLocationFilterProxyModel::DiveLocationFilterProxyModel(QObject *parent)
{
}

DiveLocationLineEdit *location_line_edit = 0;

bool DiveLocationFilterProxyModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
	if (source_row == 0)
		return true;

	QString sourceString = sourceModel()->index(source_row, DiveLocationModel::NAME).data(Qt::DisplayRole).toString();
	return sourceString.toLower().startsWith(location_line_edit->text().toLower());
}

bool DiveLocationFilterProxyModel::lessThan(const QModelIndex &source_left, const QModelIndex &source_right) const
{
	return source_left.data().toString() <= source_right.data().toString();
}


DiveLocationModel::DiveLocationModel(QObject *o)
{
	resetModel();
}

void DiveLocationModel::resetModel()
{
	beginResetModel();
	endResetModel();
}

QVariant DiveLocationModel::data(const QModelIndex &index, int role) const
{
	static const QIcon plusIcon(":plus");
	static const QIcon geoCode(":geocode");

	if (index.row() <= 1) { // two special cases.
		if (index.column() == UUID) {
			return RECENTLY_ADDED_DIVESITE;
		}
		switch (role) {
		case Qt::DisplayRole:
			return new_ds_value[index.row()];
		case Qt::ToolTipRole:
			return displayed_dive_site.uuid ?
				tr("Create a new dive site, copying relevant information from the current dive.") :
				tr("Create a new dive site with this name");
		case Qt::DecorationRole:
			return plusIcon;
		}
	}

	// The dive sites are -2 because of the first two items.
	struct dive_site *ds = get_dive_site(index.row() - 2);
	switch (role) {
	case Qt::EditRole:
	case Qt::DisplayRole:
		switch (index.column()) {
		case UUID:
			return ds->uuid;
		case NAME:
			return ds->name;
		case LATITUDE:
			return ds->latitude.udeg;
		case LONGITUDE:
			return ds->longitude.udeg;
		case DESCRIPTION:
			return ds->description;
		case NOTES:
			return ds->name;
		}
		break;
	case Qt::DecorationRole: {
		if (dive_site_has_gps_location(ds))
			return geoCode;
	}
	}
	return QVariant();
}

int DiveLocationModel::columnCount(const QModelIndex &parent) const
{
	return COLUMNS;
}

int DiveLocationModel::rowCount(const QModelIndex &parent) const
{
	return dive_site_table.nr + 2;
}

bool DiveLocationModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
	if (!index.isValid())
		return false;
	if (index.row() > 1)
		return false;

	new_ds_value[index.row()] = value.toString();

	dataChanged(index, index);
	return true;
}

DiveLocationLineEdit::DiveLocationLineEdit(QWidget *parent) : QLineEdit(parent),
							      proxy(new DiveLocationFilterProxyModel()),
							      model(new DiveLocationModel()),
							      view(new DiveLocationListView()),
							      currType(NO_DIVE_SITE)
{
	currUuid = 0;
	location_line_edit = this;

	proxy->setSourceModel(model);
	proxy->setFilterKeyColumn(DiveLocationModel::NAME);

	view->setModel(proxy);
	view->setModelColumn(DiveLocationModel::NAME);
	view->setItemDelegate(new LocationFilterDelegate());
	view->setEditTriggers(QAbstractItemView::NoEditTriggers);
	view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	view->setSelectionBehavior(QAbstractItemView::SelectRows);
	view->setSelectionMode(QAbstractItemView::SingleSelection);
	view->setParent(0, Qt::Popup);
	view->installEventFilter(this);
	view->setFocusPolicy(Qt::NoFocus);
	view->setFocusProxy(this);
	view->setMouseTracking(true);

	connect(this, &QLineEdit::textEdited, this, &DiveLocationLineEdit::setTemporaryDiveSiteName);
	connect(view, &QAbstractItemView::activated, this, &DiveLocationLineEdit::itemActivated);
	connect(view, &QAbstractItemView::entered, this, &DiveLocationLineEdit::entered);
	connect(view, &DiveLocationListView::currentIndexChanged, this, &DiveLocationLineEdit::currentChanged);
}

bool DiveLocationLineEdit::eventFilter(QObject *o, QEvent *e)
{
	if (e->type() == QEvent::KeyPress) {
		QKeyEvent *keyEv = (QKeyEvent *)e;

		if (keyEv->key() == Qt::Key_Escape) {
			view->hide();
			return true;
		}

		if (keyEv->key() == Qt::Key_Return ||
		    keyEv->key() == Qt::Key_Enter) {
#if __APPLE__
			// for some reason it seems like on a Mac hitting return/enter
			// doesn't call 'activated' for that index. so let's do it manually
			if (view->currentIndex().isValid())
				itemActivated(view->currentIndex());
#endif
			view->hide();
			return false;
		}

		if (keyEv->key() == Qt::Key_Tab) {
			itemActivated(view->currentIndex());
			view->hide();
			return false;
		}
		event(e);
	} else if (e->type() == QEvent::MouseButtonPress) {
		if (!view->underMouse()) {
			view->hide();
			return true;
		}
	}

	return false;
}

void DiveLocationLineEdit::focusOutEvent(QFocusEvent *ev)
{
	if (!view->isVisible()) {
		QLineEdit::focusOutEvent(ev);
	}
}

void DiveLocationLineEdit::itemActivated(const QModelIndex &index)
{
	QModelIndex idx = index;
	if (index.column() == DiveLocationModel::UUID)
		idx = index.model()->index(index.row(), DiveLocationModel::NAME);

	QModelIndex uuidIndex = index.model()->index(index.row(), DiveLocationModel::UUID);
	uint32_t uuid = uuidIndex.data().toInt();
	currType = uuid == 1 ? NEW_DIVE_SITE : EXISTING_DIVE_SITE;
	currUuid = uuid;
	setText(idx.data().toString());
	if (currUuid == NEW_DIVE_SITE)
		qDebug() << "Setting a New dive site";
	else
		qDebug() << "Setting a Existing dive site";
	if (view->isVisible())
		view->hide();
	emit diveSiteSelected(currUuid);
}

void DiveLocationLineEdit::refreshDiveSiteCache()
{
	model->resetModel();
}

static struct dive_site *get_dive_site_name_start_which_str(const QString &str)
{
	struct dive_site *ds;
	int i;
	for_each_dive_site (i, ds) {
		QString dsName(ds->name);
		if (dsName.toLower().startsWith(str.toLower())) {
			return ds;
		}
	}
	return NULL;
}

void DiveLocationLineEdit::setTemporaryDiveSiteName(const QString &s)
{
	QModelIndex i0 = model->index(0, DiveLocationModel::NAME);
	QModelIndex i1 = model->index(1, DiveLocationModel::NAME);
	model->setData(i0, text());

	QString i1_name = INVALID_DIVE_SITE_NAME;
	if (struct dive_site *ds = get_dive_site_name_start_which_str(text())) {
		const QString orig_name = QString(ds->name).toLower();
		const QString new_name = text().toLower();
		if (new_name != orig_name)
			i1_name = QString(ds->name);
	}

	model->setData(i1, i1_name);
	proxy->invalidate();
	fixPopupPosition();
	if (!view->isVisible())
		view->show();
}

void DiveLocationLineEdit::keyPressEvent(QKeyEvent *ev)
{
	QLineEdit::keyPressEvent(ev);
	if (ev->key() != Qt::Key_Left &&
	    ev->key() != Qt::Key_Right &&
	    ev->key() != Qt::Key_Escape &&
	    ev->key() != Qt::Key_Return) {

		if (ev->key() != Qt::Key_Up && ev->key() != Qt::Key_Down) {
			currType = NEW_DIVE_SITE;
			currUuid = RECENTLY_ADDED_DIVESITE;
		} else {
			showPopup();
		}
	} else if (ev->key() == Qt::Key_Escape) {
		view->hide();
	}
}

void DiveLocationLineEdit::fixPopupPosition()
{
	const QRect screen = QApplication::desktop()->availableGeometry(this);
	const int maxVisibleItems = 5;
	QPoint pos;
	int rh, w;
	int h = (view->sizeHintForRow(0) * qMin(maxVisibleItems, view->model()->rowCount()) + 3) + 3;
	QScrollBar *hsb = view->horizontalScrollBar();
	if (hsb && hsb->isVisible())
		h += view->horizontalScrollBar()->sizeHint().height();

	rh = height();
	pos = mapToGlobal(QPoint(0, height() - 2));
	w = width();

	if (w > screen.width())
		w = screen.width();
	if ((pos.x() + w) > (screen.x() + screen.width()))
		pos.setX(screen.x() + screen.width() - w);
	if (pos.x() < screen.x())
		pos.setX(screen.x());

	int top = pos.y() - rh - screen.top() + 2;
	int bottom = screen.bottom() - pos.y();
	h = qMax(h, view->minimumHeight());
	if (h > bottom) {
		h = qMin(qMax(top, bottom), h);
		if (top > bottom)
			pos.setY(pos.y() - h - rh + 2);
	}

	view->setGeometry(pos.x(), pos.y(), w, h);
	if (!view->currentIndex().isValid() && view->model()->rowCount()) {
		view->setCurrentIndex(view->model()->index(0, 0));
	}
}

void DiveLocationLineEdit::setCurrentDiveSiteUuid(uint32_t uuid)
{
	currUuid = uuid;
	if (uuid == 0) {
		currType = NO_DIVE_SITE;
	}
	struct dive_site *ds = get_dive_site_by_uuid(uuid);
	if (!ds)
		clear();
	else
		setText(ds->name);
}

void DiveLocationLineEdit::showPopup()
{
	fixPopupPosition();
	if (!view->isVisible()) {
		setTemporaryDiveSiteName(text());
		proxy->invalidate();
		view->show();
	}
}

DiveLocationLineEdit::DiveSiteType DiveLocationLineEdit::currDiveSiteType() const
{
	return currType;
}

uint32_t DiveLocationLineEdit::currDiveSiteUuid() const
{
	return currUuid;
}

DiveLocationListView::DiveLocationListView(QWidget *parent)
{
}

void DiveLocationListView::currentChanged(const QModelIndex &current, const QModelIndex &previous)
{
	QListView::currentChanged(current, previous);
	emit currentIndexChanged(current);
}
