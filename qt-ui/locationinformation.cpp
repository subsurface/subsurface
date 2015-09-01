#include "locationinformation.h"
#include "dive.h"
#include "mainwindow.h"
#include "divelistview.h"
#include "qthelper.h"
#include "globe.h"
#include "filtermodels.h"
#include "divelocationmodel.h"
#include "divesitehelpers.h"

#include <QDebug>
#include <QShowEvent>
#include <QItemSelectionModel>

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

bool LocationInformationWidget::eventFilter(QObject*, QEvent *ev)
{
	if( ev->type() == QEvent::ContextMenu ) {
		if (ui.diveSiteListView->selectionModel()->selectedIndexes().count() >= 2) {
			QContextMenuEvent *ctx = (QContextMenuEvent*) ev;
			QMenu contextMenu;
			contextMenu.addAction(tr("Merge dive Sites"), this, SLOT(merge_dive_sites()));
			contextMenu.exec(ctx->globalPos());
		}
	}
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
	if (displayed_dive_site.latitude.udeg || displayed_dive_site.longitude.udeg)
		ui.diveSiteCoordinates->setText(printGPSCoords(displayed_dive_site.latitude.udeg, displayed_dive_site.longitude.udeg));
	else
		ui.diveSiteCoordinates->clear();

	emit startFilterDiveSite(displayed_dive_site.uuid);
	emit startEditDiveSite(displayed_dive_site.uuid);
}

void LocationInformationWidget::updateGpsCoordinates()
{
	QString oldText = ui.diveSiteCoordinates->text();
	ui.diveSiteCoordinates->setText(printGPSCoords(displayed_dive_site.latitude.udeg, displayed_dive_site.longitude.udeg));
	if (oldText != ui.diveSiteCoordinates->text())
		markChangedWidget(ui.diveSiteCoordinates);
}

void LocationInformationWidget::acceptChanges()
{
	emit stopFilterDiveSite();
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
		QSortFilterProxyModel *m = qobject_cast<QSortFilterProxyModel*>(ui.diveSiteListView->model());
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
	p.setBrush(QPalette::Base, (l <= 0.3) ? QColor(Qt::yellow).lighter()
		: (l <= 0.6) ? QColor(Qt::yellow).light()
		: /* else */ QColor(Qt::yellow).darker(300));
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

void LocationInformationWidget::on_diveSiteCoordinates_textChanged(const QString& text)
{
	uint lat = displayed_dive_site.latitude.udeg;
	uint lon = displayed_dive_site.longitude.udeg;
	if (!same_string(qPrintable(text), printGPSCoords(lat, lon))) {
		double latitude, longitude;
		if (parseGpsText(text, &latitude, &longitude)) {
			displayed_dive_site.latitude.udeg = latitude * 1000000;
			displayed_dive_site.longitude.udeg = longitude * 1000000;
			markChangedWidget(ui.diveSiteCoordinates);
			emit coordinatesChanged();
		}
	}
}

void LocationInformationWidget::on_diveSiteDescription_textChanged(const QString& text)
{
	if (!same_string(qPrintable(text), displayed_dive_site.description))
		markChangedWidget(ui.diveSiteDescription);
}

void LocationInformationWidget::on_diveSiteName_textChanged(const QString& text)
{
	if (!same_string(qPrintable(text), displayed_dive_site.name))
		markChangedWidget(ui.diveSiteName);
}

void LocationInformationWidget::on_diveSiteNotes_textChanged()
{
	if (!same_string(qPrintable(ui.diveSiteNotes->toPlainText()),  displayed_dive_site.notes))
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

bool LocationManagementEditHelper::eventFilter(QObject *obj, QEvent *ev)
{
	QListView *view = qobject_cast<QListView*>(obj);
	if(!view)
		return false;

	if(ev->type() == QEvent::Show) {
		last_uuid = 0;
		qDebug() << "EventFilter: " << last_uuid;
	}

	if(ev->type() == QEvent::KeyPress) {
		QKeyEvent *keyEv = (QKeyEvent*) ev;
		if(keyEv->key() == Qt::Key_Return) {
			handleActivation(view->currentIndex());
			view->hide();
			return true;
		}
	}
	return false;
}

void LocationManagementEditHelper::handleActivation(const QModelIndex& activated)
{
	if (!activated.isValid())
		return;
	QModelIndex  uuidIdx = activated.model()->index(
		activated.row(), LocationInformationModel::UUID);
	last_uuid = uuidIdx.data().toInt();

	/* if we are in 'recently added divesite mode, create a new divesite,
	 * and go to dive site edit edit mode. */
	if (last_uuid == RECENTLY_ADDED_DIVESITE) {
		uint32_t ds_uuid = create_dive_site_from_current_dive(qPrintable(activated.data().toString()));
		qDebug() << "ds_uuid" << ds_uuid;
		struct dive_site *ds = get_dive_site_by_uuid(ds_uuid);
		copy_dive_site(ds, &displayed_dive_site);
		displayed_dive.dive_site_uuid = ds->uuid;
		last_uuid = ds->uuid;
		// Move this out of here later.
		MainWindow::instance()->startDiveSiteEdit();
	}

	qDebug() << "Selected dive_site: " << last_uuid;
}

void LocationManagementEditHelper::resetDiveSiteUuid() {
	last_uuid = 0;
	qDebug() << "Reset: " << last_uuid;
}

uint32_t LocationManagementEditHelper::diveSiteUuid() const {
	return last_uuid;
}

void LocationInformationWidget::reverseGeocode()
{
	ReverseGeoLookupThread *geoLookup = ReverseGeoLookupThread::instance();
	geoLookup->lookup(&displayed_dive_site);
	updateLabels();
}
