#include "locationinformation.h"
#include "dive.h"
#include "mainwindow.h"
#include "divelistview.h"
#include "qthelper.h"

#include <QDebug>
#include <QShowEvent>

LocationInformationModel::LocationInformationModel(QObject *obj)
{
}

int LocationInformationModel::rowCount(const QModelIndex &parent) const
{

}

QVariant LocationInformationModel::data(const QModelIndex &index, int role) const
{
	if (!index.isValid())
		return QVariant();
	struct dive_site *ds = get_dive_site(index.row());

	switch(role) {
		case Qt::DisplayRole : return qPrintable(ds->name);
	}

	return QVariant();
}

void LocationInformationModel::update()
{
	int i;
	struct dive_site *ds;
	for_each_dive_site (i, ds);

	if (rowCount()) {
		beginRemoveRows(QModelIndex(), 0, rowCount());
		endRemoveRows();
	}
	if (i) {
		beginInsertRows(QModelIndex(), 0, i);
		internalRowCount = i;
		endRemoveRows();
	}
}

LocationInformationWidget::LocationInformationWidget(QWidget *parent) : QGroupBox(parent), modified(false)
{
	ui.setupUi(this);
	ui.diveSiteMessage->setCloseButtonVisible(false);
	ui.diveSiteMessage->show();

	// create the three buttons and only show the close button for now
	closeAction = new QAction(tr("Close"), this);
	connect(closeAction, SIGNAL(triggered(bool)), this, SLOT(rejectChanges()));

	acceptAction = new QAction(tr("Apply changes"), this);
	connect(acceptAction, SIGNAL(triggered(bool)), this, SLOT(acceptChanges()));

	rejectAction = new QAction(tr("Discard changes"), this);
	connect(rejectAction, SIGNAL(triggered(bool)), this, SLOT(rejectChanges()));

	ui.diveSiteMessage->setText(tr("Dive site management"));
	ui.diveSiteMessage->addAction(closeAction);

}

void LocationInformationWidget::setLocationId(uint32_t uuid)
{
	currentDs = get_dive_site_by_uuid(uuid);

	if (!currentDs) {
		currentDs = get_dive_site_by_uuid(create_dive_site(""));
		displayed_dive.dive_site_uuid = currentDs->uuid;
		ui.diveSiteName->clear();
		ui.diveSiteDescription->clear();
		ui.diveSiteNotes->clear();
		ui.diveSiteCoordinates->clear();
	}
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
}

void LocationInformationWidget::updateGpsCoordinates()
{
	ui.diveSiteCoordinates->setText(printGPSCoords(displayed_dive_site.latitude.udeg, displayed_dive_site.longitude.udeg));
	MainWindow::instance()->setApplicationState("EditDiveSite");
}

void LocationInformationWidget::acceptChanges()
{
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
	if (dive_site_is_empty(currentDs)) {
		delete_dive_site(currentDs->uuid);
		displayed_dive.dive_site_uuid = 0;
		setLocationId(0);
	} else {
		setLocationId(currentDs->uuid);
	}
	mark_divelist_changed(true);
	resetState();
	emit informationManagementEnded();
}

void LocationInformationWidget::rejectChanges()
{
	Q_ASSERT(currentDs != NULL);
	if (dive_site_is_empty(currentDs)) {
		delete_dive_site(currentDs->uuid);
		displayed_dive.dive_site_uuid = 0;
		setLocationId(0);
	} else {
		setLocationId(currentDs->uuid);
	}
	resetState();
	emit informationManagementEnded();
}

void LocationInformationWidget::showEvent(QShowEvent *ev) {
	ui.diveSiteMessage->setCloseButtonVisible(false);
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
	ui.diveSiteMessage->addAction(closeAction);
	ui.diveSiteMessage->removeAction(acceptAction);
	ui.diveSiteMessage->removeAction(rejectAction);
	ui.diveSiteMessage->setCloseButtonVisible(false);
}

void LocationInformationWidget::enableEdition()
{
	MainWindow::instance()->dive_list()->setEnabled(false);
	MainWindow::instance()->setEnabledToolbar(false);
	ui.diveSiteMessage->setText(tr("You are editing a dive site"));
	ui.diveSiteMessage->removeAction(closeAction);
	ui.diveSiteMessage->addAction(acceptAction);
	ui.diveSiteMessage->addAction(rejectAction);
	ui.diveSiteMessage->setCloseButtonVisible(false);
}

void LocationInformationWidget::on_diveSiteCoordinates_textChanged(const QString& text)
{
	if (!same_string(qPrintable(text), printGPSCoords(currentDs->latitude.udeg, currentDs->longitude.udeg)))
		markChangedWidget(ui.diveSiteCoordinates);
}

void LocationInformationWidget::on_diveSiteDescription_textChanged(const QString& text)
{
	if (!same_string(qPrintable(text), currentDs->description))
		markChangedWidget(ui.diveSiteDescription);
}

void LocationInformationWidget::on_diveSiteName_textChanged(const QString& text)
{
	if (!same_string(qPrintable(text), currentDs->name))
		markChangedWidget(ui.diveSiteName);
}

void LocationInformationWidget::on_diveSiteNotes_textChanged()
{
	if (!same_string(qPrintable(ui.diveSiteNotes->toPlainText()),  currentDs->notes))
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
