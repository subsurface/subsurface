#include "locationinformation.h"
#include "dive.h"
#include "mainwindow.h"
#include "divelistview.h"
#include "qthelper.h"
#include "globe.h"
#include "filtermodels.h"
#include "divelocationmodel.h"
#include <QDebug>
#include <QShowEvent>

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
}

void LocationInformationWidget::setCurrentDiveSiteByUuid(uint32_t uuid)
{
	currentDs = get_dive_site_by_uuid(uuid);
	if(!currentDs)
		return;

	displayed_dive_site = *currentDs;

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

	if (current_mode == EDIT_DIVE_SITE)
		emit startFilterDiveSite(displayed_dive_site.uuid);
	emit startEditDiveSite(uuid);
}

void LocationInformationWidget::updateGpsCoordinates()
{
	ui.diveSiteCoordinates->setText(printGPSCoords(displayed_dive_site.latitude.udeg, displayed_dive_site.longitude.udeg));
}

void LocationInformationWidget::acceptChanges()
{
	emit stopFilterDiveSite();
	char *uiString;
	currentDs->latitude = displayed_dive_site.latitude;
	currentDs->longitude = displayed_dive_site.longitude;
	uiString = ui.diveSiteName->text().toUtf8().data();
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
	if (current_mode == CREATE_DIVE_SITE)
		displayed_dive.dive_site_uuid = currentDs->uuid;
	if (dive_site_is_empty(currentDs)) {
		LocationInformationModel::instance()->removeRow(get_divesite_idx(currentDs));
		displayed_dive.dive_site_uuid = 0;
	}

	mark_divelist_changed(true);
	resetState();
	emit informationManagementEnded();
	emit coordinatesChanged();
}

void LocationInformationWidget::editDiveSite(uint32_t uuid)
{
	current_mode = EDIT_DIVE_SITE;
	setCurrentDiveSiteByUuid(uuid);
}

void LocationInformationWidget::createDiveSite()
{
	uint32_t uid = LocationInformationModel::instance()->addDiveSite(tr("untitled"));
	current_mode = CREATE_DIVE_SITE;
	setCurrentDiveSiteByUuid(uid);
}

void LocationInformationWidget::rejectChanges()
{
	if (current_mode == CREATE_DIVE_SITE) {
		LocationInformationModel::instance()->removeRow(get_divesite_idx(currentDs));
		if (displayed_dive.dive_site_uuid) {
			displayed_dive_site = *get_dive_site_by_uuid(displayed_dive.dive_site_uuid);
		} else {
			displayed_dive_site.uuid = 0;
		}
	} else if ((currentDs && dive_site_is_empty(currentDs))) {
		LocationInformationModel::instance()->removeRow(get_divesite_idx(currentDs));
		displayed_dive_site.uuid = 0;
	}

	resetState();
	emit stopFilterDiveSite();
	emit informationManagementEnded();
	emit coordinatesChanged();
}

void LocationInformationWidget::showEvent(QShowEvent *ev)
{
	if (displayed_dive_site.uuid && current_mode == EDIT_DIVE_SITE)
		emit startFilterDiveSite(displayed_dive_site.uuid);
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
	if (displayed_dive.id) {
		struct dive_site *ds = get_dive_site_by_uuid(displayed_dive.dive_site_uuid);
		if(ds) {
			displayed_dive_site = *ds;
		}
	}
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

extern bool parseGpsText(const QString &gps_text, double *latitude, double *longitude);

void LocationInformationWidget::on_diveSiteCoordinates_textChanged(const QString& text)
{
	if (!currentDs || !same_string(qPrintable(text), printGPSCoords(currentDs->latitude.udeg, currentDs->longitude.udeg))) {
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
	if (!currentDs || !same_string(qPrintable(text), currentDs->description))
		markChangedWidget(ui.diveSiteDescription);
}

void LocationInformationWidget::on_diveSiteName_textChanged(const QString& text)
{
	if (currentDs && text != currentDs->name) {
		// This needs to be changed directly into the model so that
		// the changes are replyed on the ComboBox with the current selection.

		int i;
		struct dive_site *ds;
		for_each_dive_site(i,ds)
			if (ds->uuid == currentDs->uuid)
				break;

		displayed_dive_site.name = copy_string(qPrintable(text));
		QModelIndex idx = LocationInformationModel::instance()->index(i,0);
		LocationInformationModel::instance()->setData(idx, text, Qt::EditRole);
		markChangedWidget(ui.diveSiteName);
		emit coordinatesChanged();
	}
}

void LocationInformationWidget::on_diveSiteNotes_textChanged()
{
	if (! currentDs || !same_string(qPrintable(ui.diveSiteNotes->toPlainText()),  currentDs->notes))
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

SimpleDiveSiteEditDialog::SimpleDiveSiteEditDialog(QWidget *parent) :
	QDialog(parent,  Qt::FramelessWindowHint | Qt::WindowSystemMenuHint | Qt::Popup),
	ui(new Ui::SimpleDiveSiteEditDialog())
{
	ui->setupUi(this);
}

SimpleDiveSiteEditDialog::~SimpleDiveSiteEditDialog()
{
	delete ui;
}

void SimpleDiveSiteEditDialog::showEvent(QShowEvent *ev)
{
	const int heigth = 190;
	const int width = 280;

	QDialog::showEvent(ev);
	QRect currGeometry = geometry();
	currGeometry.setX(QCursor::pos().x() + 10);
	currGeometry.setY(QCursor::pos().y() - heigth / 2);
	currGeometry.setWidth(width);
	currGeometry.setHeight(heigth);
	setGeometry(currGeometry);
	ev->accept();
}
