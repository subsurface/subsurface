// SPDX-License-Identifier: GPL-2.0
#include "desktop-widgets/locationinformation.h"
#include "desktop-widgets/importgps.h"
#include "core/subsurface-string.h"
#include "desktop-widgets/mainwindow.h"
#include "desktop-widgets/divelistview.h"
#include "core/qthelper.h"
#include "desktop-widgets/mapwidget.h"
#include "core/color.h"
#include "core/divefilter.h"
#include "core/divelog.h"
#include "core/divesite.h"
#include "core/divesitehelpers.h"
#include "desktop-widgets/modeldelegates.h"
#include "core/subsurface-qt/divelistnotifier.h"
#include "core/taxonomy.h"
#include "core/selection.h"
#include "core/settings/qPrefUnit.h"
#include "commands/command.h"

#include <QShowEvent>
#include <QItemSelectionModel>
#include <qmessagebox.h>
#include <cstdlib>
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <QDesktopWidget>
#endif
#include <QFileDialog>
#include <QScrollBar>

LocationInformationWidget::LocationInformationWidget(QWidget *parent) : QGroupBox(parent), diveSite(nullptr), closeDistance(0)
{
	ui.setupUi(this);
	ui.diveSiteMessage->setCloseButtonVisible(false);

	QAction *acceptAction = new QAction(tr("Done"), this);
	connect(acceptAction, &QAction::triggered, this, &LocationInformationWidget::acceptChanges);

	ui.diveSiteMessage->setText(tr("Dive site management"));
	ui.diveSiteMessage->addAction(acceptAction);

	connect(ui.geoCodeButton, &QPushButton::clicked, this, &LocationInformationWidget::reverseGeocode);
	ui.diveSiteCoordinates->installEventFilter(this);

	connect(&diveListNotifier, &DiveListNotifier::diveSiteChanged, this, &LocationInformationWidget::diveSiteChanged);
	connect(&diveListNotifier, &DiveListNotifier::diveSiteDeleted, this, &LocationInformationWidget::diveSiteDeleted);
	connect(qPrefUnits::instance(), &qPrefUnits::unit_systemChanged, this, &LocationInformationWidget::unitsChanged);

	ui.diveSiteListView->setModel(&filter_model);
	ui.diveSiteListView->setModelColumn(LocationInformationModel::NAME);
	ui.diveSiteListView->installEventFilter(this);
}

void LocationInformationWidget::keyPressEvent(QKeyEvent *e)
{
	if (e->key() == Qt::Key_Escape)
		MainWindow::instance()->setFocus();
	return QGroupBox::keyPressEvent(e);
}

bool LocationInformationWidget::eventFilter(QObject *, QEvent *ev)
{
	if (ev->type() == QEvent::ContextMenu) {
		QContextMenuEvent *ctx = (QContextMenuEvent *)ev;
		QMenu contextMenu;
		contextMenu.addAction(tr("Merge into current site"), this, &LocationInformationWidget::mergeSelectedDiveSites);
		const QModelIndexList selection = ui.diveSiteListView->selectionModel()->selectedIndexes();
		if (selection.count() == 1)
			contextMenu.addAction(tr("Merge current site into this site"), this, &LocationInformationWidget::mergeIntoSelectedDiveSite);
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

	const QModelIndexList selection = ui.diveSiteListView->selectionModel()->selectedIndexes();
	QVector<dive_site *> selected_dive_sites;
	selected_dive_sites.reserve(selection.count());
	for (const QModelIndex &idx: selection) {
		dive_site *ds = idx.data(LocationInformationModel::DIVESITE_ROLE).value<dive_site *>();
		if (ds)
			selected_dive_sites.push_back(ds);
	}
	Command::mergeDiveSites(diveSite, selected_dive_sites);
}

void LocationInformationWidget::mergeIntoSelectedDiveSite()
{
	if (!diveSite)
		return;

	const QModelIndexList selection = ui.diveSiteListView->selectionModel()->selectedIndexes();
	if (selection.count() != 1)
		return;

	dive_site *selected_dive_site = selection[0].data(LocationInformationModel::DIVESITE_ROLE).value<dive_site *>();
	if (!selected_dive_site)
		return;

	QVector<dive_site *> dive_sites;
	dive_sites.push_back(diveSite);
	Command::mergeDiveSites(selected_dive_site, dive_sites);
}

// If we can't parse the coordinates, inform the user with a visual clue
void LocationInformationWidget::coordinatesSetWarning(bool warn)
{
	QPalette palette;
	if (warn) {
		palette.setColor(QPalette::Base, REDORANGE1_MED_TRANS);
		palette.setColor(QPalette::Text, WHITE1);
	}
	ui.diveSiteCoordinates->setPalette(palette);
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
	coordinatesSetWarning(false);

	ui.locationTags->setText(constructLocationTags(&diveSite->taxonomy, false));
}

void LocationInformationWidget::unitsChanged()
{
	if (prefs.units.length == units::METERS) {
		ui.diveSiteDistanceUnits->setText("m");
		ui.diveSiteDistance->setText(QString::number(lrint(closeDistance / 1000.0)));
	} else {
		ui.diveSiteDistanceUnits->setText("ft");
		ui.diveSiteDistance->setText(QString::number(lrint(mm_to_feet(closeDistance))));
	}
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
		coordinatesSetWarning(false);
		return;
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
	coordinatesSetWarning(false);
	ui.locationTags->clear();
}

// Parse GPS text into location_t
static location_t parseGpsText(const QString &text)
{
	double lat, lon;
	if (parseGpsText(text.trimmed(), &lat, &lon))
		return create_location(lat, lon);
	return zero_location;
}

// Check if GPS text is parseable
static bool validateGpsText(const QString &textIn)
{
	double lat, lon;
	QString text = textIn.trimmed();
	return text.isEmpty() || parseGpsText(text.trimmed(), &lat, &lon);
}

void LocationInformationWidget::diveSiteDeleted(struct dive_site *ds, int)
{
	// If the currently edited dive site was removed under our feet, close the widget.
	// This will reset the dangling pointer.
	if (ds && ds == diveSite)
		acceptChanges();
}

void LocationInformationWidget::acceptChanges()
{
	closeDistance = 0;

	MainWindow::instance()->diveList->setEnabled(true);
	MainWindow::instance()->setEnabledToolbar(true);
	MainWindow::instance()->enterPreviousState();
	DiveFilter::instance()->stopFilterDiveSites();

	// Subtlety alert: diveSite must be cleared *after* exiting the dive-site mode.
	// Exiting dive-site mode removes the focus from the active widget and
	// thus fires the corresponding editingFinished signal, which in turn creates
	// an undo-command. To set an undo-command, the widget has to know the
	// currently edited dive-site.
	diveSite = nullptr;
}

void LocationInformationWidget::initFields(dive_site *ds)
{
	diveSite = ds;
	if (ds) {
		filter_model.set(ds, ds->location);
		updateLabels();
		enableLocationButtons(dive_site_has_gps_location(ds));
		DiveFilter::instance()->startFilterDiveSites(QVector<dive_site *>{ ds });
		filter_model.invalidate();
	} else {
		filter_model.set(0, zero_location);
		clearLabels();
	}

	unitsChanged();
}

void LocationInformationWidget::on_GPSbutton_clicked()
{
	QFileInfo finfo(system_default_directory());
	QString fileName = QFileDialog::getOpenFileName(this,
							tr("Select GPS file to open"),
							finfo.absolutePath(),
							tr("GPS files (*.gpx *.GPX)"));
	if (fileName.isEmpty())
		return;

	ImportGPS GPSDialog(this, fileName, &ui); // Create a GPS import QDialog
	GPSDialog.coords.start_dive = current_dive->when; // initialise
	GPSDialog.coords.end_dive = dive_endtime(current_dive);
	if (getCoordsFromGPXFile(&GPSDialog.coords, fileName) == 0) { // Get coordinates from GPS file
		GPSDialog.updateUI();         // If successful, put results in Dialog
		if (!GPSDialog.exec())        // and show QDialog
			return;
	}
}

void LocationInformationWidget::on_diveSiteCoordinates_editingFinished()
{
	if (diveSite && validateGpsText(ui.diveSiteCoordinates->text()))
		Command::editDiveSiteLocation(diveSite, parseGpsText(ui.diveSiteCoordinates->text()));
}

void LocationInformationWidget::on_diveSiteCoordinates_textEdited(const QString &s)
{
	coordinatesSetWarning(!validateGpsText(s));
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

void LocationInformationWidget::on_diveSiteDistance_textChanged(const QString &s)
{
	bool ok;
	uint64_t d = s.toLongLong(&ok);
	if (!ok)
		d = 0;
	closeDistance = prefs.units.length == units::METERS ? d * 1000 : feet_to_mm(d);
	filter_model.setDistance(closeDistance);
}

void LocationInformationWidget::reverseGeocode()
{
	dive_site *ds = diveSite; /* Save local copy; possibility of user closing the widget while reverseGeoLookup is running (see #2930) */
	location_t location = parseGpsText(ui.diveSiteCoordinates->text());
	if (!ds || !has_location(&location))
		return;
	taxonomy_data taxonomy = reverseGeoLookup(location.lat, location.lon);
	if (ds != diveSite) {
		free_taxonomy(&taxonomy);
		return;
	}
	// This call transfers ownership of the taxonomy memory into an EditDiveSiteTaxonomy object
	Command::editDiveSiteTaxonomy(ds, taxonomy);
}

DiveLocationFilterProxyModel::DiveLocationFilterProxyModel(QObject *) : currentLocation(zero_location)
{
}

void DiveLocationFilterProxyModel::setFilter(const QString &filterIn)
{
	filter = filterIn;
	invalidate();
	sort(LocationInformationModel::NAME);
}

void DiveLocationFilterProxyModel::setCurrentLocation(location_t loc)
{
	currentLocation = loc;
	sort(LocationInformationModel::NAME);
}

bool DiveLocationFilterProxyModel::filterAcceptsRow(int source_row, const QModelIndex&) const
{
	// We don't want to show the first two entries (add dive site with that name)
	// if there is no filter text.
	if (filter.isEmpty() && source_row <= 1)
		return false;

	if (source_row == 0)
		return true;

	QString sourceString = sourceModel()->index(source_row, LocationInformationModel::NAME).data(Qt::DisplayRole).toString();
	return sourceString.contains(filter, Qt::CaseInsensitive);
}

bool DiveLocationFilterProxyModel::lessThan(const QModelIndex &source_left, const QModelIndex &source_right) const
{
	// The first two entries are special - we never want to change their order
	if (source_left.row() <= 1 || source_right.row() <= 1)
		return source_left.row() < source_right.row();

	// If there is a current location, sort by that - otherwise use the provided column
	if (has_location(&currentLocation)) {
		// The dive sites are -2 because of the first two items.
		struct dive_site *ds1 = get_dive_site(source_left.row() - 2, divelog.sites);
		struct dive_site *ds2 = get_dive_site(source_right.row() - 2, divelog.sites);
		return get_distance(&ds1->location, &currentLocation) < get_distance(&ds2->location, &currentLocation);
	}
	return source_left.data().toString().compare(source_right.data().toString(), Qt::CaseInsensitive) < 0;
}

DiveLocationModel::DiveLocationModel(QObject *)
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
			return current_dive && current_dive->dive_site ?
				tr("Create a new dive site, copying relevant information from the current dive.") :
				tr("Create a new dive site with this name");
		case Qt::DecorationRole:
			return plusIcon;
		}
		return QVariant();
	}

	// The dive sites are -2 because of the first two items.
	struct dive_site *ds = get_dive_site(index.row() - 2, divelog.sites);
	return LocationInformationModel::getDiveSiteData(ds, index.column(), role);
}

int DiveLocationModel::columnCount(const QModelIndex&) const
{
	return LocationInformationModel::COLUMNS;
}

int DiveLocationModel::rowCount(const QModelIndex&) const
{
	return divelog.sites->nr + 2;
}

Qt::ItemFlags DiveLocationModel::flags(const QModelIndex &index) const
{
	// This is crazy: If an entry is not marked as editable, the QListView
	// (or rather the QAbstractItemView base class) clears the WA_InputMethod
	// flag, which means that key-composition events are disabled. This
	// breaks composition as long as the popup is openen. Therefore,
	// make all items editable.
	return QAbstractItemModel::flags(index) | Qt::ItemIsEditable;
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
							      proxy(new DiveLocationFilterProxyModel),
							      model(new DiveLocationModel),
							      view(new DiveLocationListView),
							      currDs(nullptr)
{
	proxy->setSourceModel(model);
	proxy->setFilterKeyColumn(LocationInformationModel::NAME);

	view->setModel(proxy);
	view->setModelColumn(LocationInformationModel::NAME);
	view->setItemDelegate(&delegate);
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

bool DiveLocationLineEdit::eventFilter(QObject *, QEvent *e)
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
	} else if (e->type() == QEvent::InputMethod) {
		inputMethodEvent(static_cast<QInputMethodEvent *>(e));
	}

	return false;
}

void DiveLocationLineEdit::focusOutEvent(QFocusEvent *ev)
{
	if (!view->isVisible())
		QLineEdit::focusOutEvent(ev);
}

void DiveLocationLineEdit::itemActivated(const QModelIndex &index)
{
	QModelIndex idx = index;
	if (index.column() == LocationInformationModel::DIVESITE)
		idx = index.model()->index(index.row(), LocationInformationModel::NAME);

	dive_site *ds = index.model()->index(index.row(), LocationInformationModel::DIVESITE).data().value<dive_site *>();
	currDs = ds;
	setText(idx.data().toString());
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
	for_each_dive_site (i, ds, divelog.sites) {
		QString dsName(ds->name);
		if (dsName.toLower().startsWith(str.toLower()))
			return ds;
	}
	return NULL;
}

void DiveLocationLineEdit::setTemporaryDiveSiteName(const QString &name)
{
	// This function fills the first two entries with potential names of
	// a dive site to be generated. The first entry is simply the entered
	// text. The second entry is the first known dive site name starting
	// with the entered text.
	QModelIndex i0 = model->index(0, LocationInformationModel::NAME);
	QModelIndex i1 = model->index(1, LocationInformationModel::NAME);
	model->setData(i0, name);

	// Note: if i1_name stays empty, the line will automatically
	// be filtered out by the proxy filter, as it does not contain
	// the user entered text.
	QString i1_name;
	if (struct dive_site *ds = get_dive_site_name_start_which_str(name)) {
		const QString orig_name = QString(ds->name).toLower();
		const QString new_name = name.toLower();
		if (new_name != orig_name)
			i1_name = QString(ds->name);
	}

	model->setData(i1, i1_name);
	proxy->setFilter(name);
	fixPopupPosition();
	if (!view->isVisible()) {
		view->show();
		// TODO: For some reason the show() call clears this flag,
		// which breaks key composition. Find the real cause for
		// this strange behavior!
		view->setAttribute(Qt::WA_InputMethodEnabled);
	}
}

void DiveLocationLineEdit::keyPressEvent(QKeyEvent *ev)
{
	QLineEdit::keyPressEvent(ev);
	if (ev->key() != Qt::Key_Left &&
	    ev->key() != Qt::Key_Right &&
	    ev->key() != Qt::Key_Escape &&
	    ev->key() != Qt::Key_Return) {

		if (ev->key() != Qt::Key_Up && ev->key() != Qt::Key_Down)
			currDs = RECENTLY_ADDED_DIVESITE;
		else
			showPopup();
	} else if (ev->key() == Qt::Key_Escape) {
		view->hide();
	}
}

void DiveLocationLineEdit::fixPopupPosition()
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
	const QRect screen = this->screen()->availableGeometry();
#else
	const QRect screen = QApplication::desktop()->availableGeometry(this);
#endif
	const int maxVisibleItems = 5;
	QPoint pos;
	int rh, w;
	int h = (view->sizeHintForRow(0) * std::min(maxVisibleItems, view->model()->rowCount()) + 3) + 3;
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
	h = std::max(h, view->minimumHeight());
	if (h > bottom) {
		h = std::min(std::max(top, bottom), h);
		if (top > bottom)
			pos.setY(pos.y() - h - rh + 2);
	}

	view->setGeometry(pos.x(), pos.y(), w, h);
	if (!view->currentIndex().isValid() && view->model()->rowCount()) {
		view->setCurrentIndex(view->model()->index(0, 1));
	}
}

void DiveLocationLineEdit::setCurrentDiveSite(struct dive *d)
{
	location_t currentLocation;
	if (d) {
		currDs = get_dive_site_for_dive(d);
		currentLocation = dive_get_gps_location(d);
	} else {
		currDs = nullptr;
		currentLocation = zero_location;
	}
	if (!currDs)
		clear();
	else
		setText(currDs->name);
	proxy->setCurrentLocation(currentLocation);
	delegate.setCurrentLocation(currentLocation);
}

void DiveLocationLineEdit::showPopup()
{
	if (!view->isVisible())
		setTemporaryDiveSiteName(text());
}

void DiveLocationLineEdit::showAllSites()
{
	if (!view->isVisible()) {
		// By setting the "temporary dive site name" to the empty string,
		// all dive sites are shown sorted by distance from the site of
		// the current dive.
		setTemporaryDiveSiteName(QString());

		// By selecting the whole text, the user can immediately start
		// typing to activate the full-text filter.
		selectAll();
	}
}

struct dive_site *DiveLocationLineEdit::currDiveSite() const
{
	// If there is no text, this corresponds to the empty dive site
	return text().trimmed().isEmpty() ? nullptr : currDs;
}

DiveLocationListView::DiveLocationListView(QWidget *parent) : QListView(parent)
{
}

void DiveLocationListView::currentChanged(const QModelIndex &current, const QModelIndex &previous)
{
	QListView::currentChanged(current, previous);
	emit currentIndexChanged(current);
}
