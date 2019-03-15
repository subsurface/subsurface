// SPDX-License-Identifier: GPL-2.0
#include "desktop-widgets/locationinformation.h"
#include "core/subsurface-string.h"
#include "desktop-widgets/mainwindow.h"
#include "desktop-widgets/divelistview.h"
#include "core/qthelper.h"
#include "desktop-widgets/mapwidget.h"
#include "qt-models/filtermodels.h"
#include "core/divesitehelpers.h"
#include "desktop-widgets/modeldelegates.h"
#include "core/subsurface-qt/DiveListNotifier.h"
#include "command.h"
#include "core/taxonomy.h"

#include <QDebug>
#include <QShowEvent>
#include <QItemSelectionModel>
#include <qmessagebox.h>
#include <cstdlib>
#include <QDesktopWidget>
#include <QScrollBar>

LocationInformationWidget::LocationInformationWidget(QWidget *parent) : QGroupBox(parent), diveSite(nullptr)
{
	ui.setupUi(this);
	ui.diveSiteMessage->setCloseButtonVisible(false);

	QAction *acceptAction = new QAction(tr("Done"), this);
	connect(acceptAction, &QAction::triggered, this, &LocationInformationWidget::acceptChanges);

	ui.diveSiteMessage->setText(tr("Dive site management"));
	ui.diveSiteMessage->addAction(acceptAction);

	connect(ui.geoCodeButton, SIGNAL(clicked()), this, SLOT(reverseGeocode()));
	ui.diveSiteCoordinates->installEventFilter(this);

	connect(&diveListNotifier, &DiveListNotifier::diveSiteChanged, this, &LocationInformationWidget::diveSiteChanged);

	ui.diveSiteListView->setModel(&filter_model);
	ui.diveSiteListView->setModelColumn(LocationInformationModel::NAME);
	ui.diveSiteListView->installEventFilter(this);
}

bool LocationInformationWidget::eventFilter(QObject *object, QEvent *ev)
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

void LocationInformationWidget::enableLocationButtons(bool enable)
{
	ui.geoCodeButton->setEnabled(enable);
}

void LocationInformationWidget::mergeSelectedDiveSites()
{
	if (!diveSite)
		return;

	QModelIndexList selection = ui.diveSiteListView->selectionModel()->selectedIndexes();
	QVector<dive_site *> selected_dive_sites;
	selected_dive_sites.reserve(selection.count());
	for (const QModelIndex &idx: selection) {
		dive_site *ds = idx.data(LocationInformationModel::DIVESITE_ROLE).value<dive_site *>();
		if (ds)
			selected_dive_sites.push_back(ds);
	}
	Command::mergeDiveSites(diveSite, selected_dive_sites);
}

void LocationInformationWidget::updateLabels()
{
	if (!diveSite) {
		clearLabels();
		return;
	}
	if (diveSite->name)
		ui.diveSiteName->setText(diveSite->name);
	else
		ui.diveSiteName->clear();
	const char *country = taxonomy_get_country(&diveSite->taxonomy);
	if (country)
		ui.diveSiteCountry->setText(country);
	else
		ui.diveSiteCountry->clear();
	if (diveSite->description)
		ui.diveSiteDescription->setText(diveSite->description);
	else
		ui.diveSiteDescription->clear();
	if (diveSite->notes)
		ui.diveSiteNotes->setPlainText(diveSite->notes);
	else
		ui.diveSiteNotes->clear();
	if (has_location(&diveSite->location))
		ui.diveSiteCoordinates->setText(printGPSCoords(&diveSite->location));
	else
		ui.diveSiteCoordinates->clear();

	ui.locationTags->setText(constructLocationTags(&diveSite->taxonomy, false));
}

void LocationInformationWidget::diveSiteChanged(struct dive_site *ds, int field)
{
	if (diveSite != ds)
		return; // A different dive site was changed -> do nothing.
	switch (field) {
	case LocationInformationModel::NAME:
		ui.diveSiteName->setText(diveSite->name);
		return;
	case LocationInformationModel::DESCRIPTION:
		ui.diveSiteDescription->setText(diveSite->description);
		return;
	case LocationInformationModel::NOTES:
		ui.diveSiteNotes->setText(diveSite->notes);
		return;
	case LocationInformationModel::TAXONOMY:
		ui.diveSiteCountry->setText(taxonomy_get_country(&diveSite->taxonomy));
		ui.locationTags->setText(constructLocationTags(&diveSite->taxonomy, false));
		return;
	case LocationInformationModel::LOCATION:
		filter_model.setCoordinates(diveSite->location);
		if (has_location(&diveSite->location)) {
			enableLocationButtons(true);
			ui.diveSiteCoordinates->setText(printGPSCoords(&diveSite->location));
		} else {
			enableLocationButtons(false);
			ui.diveSiteCoordinates->clear();
		}
	default:
		return;
	}
}

void LocationInformationWidget::clearLabels()
{
	ui.diveSiteName->clear();
	ui.diveSiteCountry->clear();
	ui.diveSiteDescription->clear();
	ui.diveSiteNotes->clear();
	ui.diveSiteCoordinates->clear();
	ui.locationTags->clear();
}

// Parse GPS text into location_t
static location_t parseGpsText(const QString &text)
{
	double lat, lon;
	if (parseGpsText(text, &lat, &lon))
		return create_location(lat, lon);
	return { {0}, {0} };
}

void LocationInformationWidget::acceptChanges()
{
	MainWindow::instance()->diveList->setEnabled(true);
	MainWindow::instance()->setEnabledToolbar(true);
	MainWindow::instance()->setApplicationState("Default");
	MapWidget::instance()->endGetDiveCoordinates();
	MapWidget::instance()->repopulateLabels();
	MultiFilterSortModel::instance()->stopFilterDiveSite();
	emit endEditDiveSite();
}

void LocationInformationWidget::initFields(dive_site *ds)
{
	diveSite = ds;
	if (ds) {
		filter_model.set(ds, ds->location);
		updateLabels();
		enableLocationButtons(dive_site_has_gps_location(ds));
		QSortFilterProxyModel *m = qobject_cast<QSortFilterProxyModel *>(ui.diveSiteListView->model());
		MultiFilterSortModel::instance()->startFilterDiveSite(ds);
		if (m)
			m->invalidate();
	} else {
		filter_model.set(0, location_t { degrees_t{ 0 }, degrees_t{ 0 } });
		clearLabels();
	}
	MapWidget::instance()->prepareForGetDiveCoordinates(ds);
}

void LocationInformationWidget::on_diveSiteCoordinates_editingFinished()
{
	if (!diveSite)
		return;
	Command::editDiveSiteLocation(diveSite, parseGpsText(ui.diveSiteCoordinates->text()));
}

void LocationInformationWidget::on_diveSiteCountry_editingFinished()
{
	if (diveSite)
		Command::editDiveSiteCountry(diveSite, ui.diveSiteCountry->text());
}

void LocationInformationWidget::on_diveSiteDescription_editingFinished()
{
	if (diveSite)
		Command::editDiveSiteDescription(diveSite, ui.diveSiteDescription->text());
}

void LocationInformationWidget::on_diveSiteName_editingFinished()
{
	if (diveSite)
		Command::editDiveSiteName(diveSite, ui.diveSiteName->text());
}

void LocationInformationWidget::on_diveSiteNotes_editingFinished()
{
	if (diveSite)
		Command::editDiveSiteNotes(diveSite, ui.diveSiteNotes->toPlainText());
}

void LocationInformationWidget::reverseGeocode()
{
	location_t location = parseGpsText(ui.diveSiteCoordinates->text());
	if (!diveSite || !has_location(&location))
		return;
	taxonomy_data taxonomy = { 0 };
	reverseGeoLookup(location.lat, location.lon, &taxonomy);
	Command::editDiveSiteTaxonomy(diveSite, taxonomy);
}

DiveLocationFilterProxyModel::DiveLocationFilterProxyModel(QObject*)
{
}

DiveLocationLineEdit *location_line_edit = 0;

bool DiveLocationFilterProxyModel::filterAcceptsRow(int source_row, const QModelIndex&) const
{
	if (source_row == 0)
		return true;

	QString sourceString = sourceModel()->index(source_row, LocationInformationModel::NAME).data(Qt::DisplayRole).toString();
	return sourceString.toLower().contains(location_line_edit->text().toLower());
}

bool DiveLocationFilterProxyModel::lessThan(const QModelIndex &source_left, const QModelIndex &source_right) const
{
	return source_left.data().toString() < source_right.data().toString();
}

DiveLocationModel::DiveLocationModel(QObject*)
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
	static const QIcon plusIcon(":list-add-icon");
	static const QIcon geoCode(":geotag-icon");

	if (index.row() <= 1) { // two special cases.
		if (index.column() == LocationInformationModel::DIVESITE)
			return QVariant::fromValue<dive_site *>(RECENTLY_ADDED_DIVESITE);
		switch (role) {
		case Qt::DisplayRole:
			return new_ds_value[index.row()];
		case Qt::ToolTipRole:
			return displayed_dive.dive_site ?
				tr("Create a new dive site, copying relevant information from the current dive.") :
				tr("Create a new dive site with this name");
		case Qt::DecorationRole:
			return plusIcon;
		}
	}

	// The dive sites are -2 because of the first two items.
	struct dive_site *ds = get_dive_site(index.row() - 2, &dive_site_table);
	return LocationInformationModel::getDiveSiteData(ds, index.column(), role);
}

int DiveLocationModel::columnCount(const QModelIndex&) const
{
	return LocationInformationModel::COLUMNS;
}

int DiveLocationModel::rowCount(const QModelIndex&) const
{
	return dive_site_table.nr + 2;
}

bool DiveLocationModel::setData(const QModelIndex &index, const QVariant &value, int)
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
							      currType(NO_DIVE_SITE),
							      currDs(nullptr)
{
	location_line_edit = this;

	proxy->setSourceModel(model);
	proxy->setFilterKeyColumn(LocationInformationModel::NAME);

	view->setModel(proxy);
	view->setModelColumn(LocationInformationModel::NAME);
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

bool DiveLocationLineEdit::eventFilter(QObject*, QEvent *e)
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
	else if (e->type() == QEvent::InputMethod) {
		this->inputMethodEvent(static_cast<QInputMethodEvent *>(e));
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
	if (index.column() == LocationInformationModel::DIVESITE)
		idx = index.model()->index(index.row(), LocationInformationModel::NAME);

	dive_site *ds = index.model()->index(index.row(), LocationInformationModel::DIVESITE).data().value<dive_site *>();
	currType = ds == RECENTLY_ADDED_DIVESITE ? NEW_DIVE_SITE : EXISTING_DIVE_SITE;
	currDs = ds;
	setText(idx.data().toString());
	if (currType == NEW_DIVE_SITE)
		qDebug() << "Setting a New dive site";
	else
		qDebug() << "Setting a Existing dive site";
	if (view->isVisible())
		view->hide();
	emit diveSiteSelected();
}

void DiveLocationLineEdit::refreshDiveSiteCache()
{
	model->resetModel();
}

static struct dive_site *get_dive_site_name_start_which_str(const QString &str)
{
	struct dive_site *ds;
	int i;
	for_each_dive_site (i, ds, &dive_site_table) {
		QString dsName(ds->name);
		if (dsName.toLower().startsWith(str.toLower())) {
			return ds;
		}
	}
	return NULL;
}

void DiveLocationLineEdit::setTemporaryDiveSiteName(const QString&)
{
	// This function fills the first two entries with potential names of
	// a dive site to be generated. The first entry is simply the entered
	// text. The second entry is the first known dive site name starting
	// with the entered text.
	QModelIndex i0 = model->index(0, LocationInformationModel::NAME);
	QModelIndex i1 = model->index(1, LocationInformationModel::NAME);
	model->setData(i0, text());

	// Note: if i1_name stays empty, the line will automatically
	// be filtered out by the proxy filter, as it does not contain
	// the user entered text.
	QString i1_name;
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
			currDs = RECENTLY_ADDED_DIVESITE;
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
		view->setCurrentIndex(view->model()->index(0, 1));
	}
}

void DiveLocationLineEdit::setCurrentDiveSite(struct dive_site *ds)
{
	currDs = ds;
	if (!currDs) {
		currType = NO_DIVE_SITE;
		clear();
	} else {
		setText(ds->name);
	}
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

struct dive_site *DiveLocationLineEdit::currDiveSite() const
{
	return currDs;
}

DiveLocationListView::DiveLocationListView(QWidget*)
{
}

void DiveLocationListView::currentChanged(const QModelIndex &current, const QModelIndex &previous)
{
	QListView::currentChanged(current, previous);
	emit currentIndexChanged(current);
}
