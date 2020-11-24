// SPDX-License-Identifier: GPL-2.0
#include "preferencesdialog.h"

#include "abstractpreferenceswidget.h"
#include "preferences_language.h"
#include "preferences_georeference.h"
#include "preferences_defaults.h"
#include "preferences_units.h"
#include "preferences_graph.h"
#include "preferences_network.h"
#include "preferences_cloud.h"
#include "preferences_equipment.h"
#include "preferences_media.h"
#include "preferences_dc.h"
#include "preferences_log.h"
#include "preferences_reset.h"

#include "core/qthelper.h"
#include "core/subsurface-qt/divelistnotifier.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QListWidget>
#include <QStackedWidget>
#include <QDialogButtonBox>
#include <QAbstractButton>

PreferencesDialog *PreferencesDialog::instance()
{
	static PreferencesDialog *self = new PreferencesDialog();
	return self;
}

static bool abstractpreferenceswidget_lessthan(const AbstractPreferencesWidget *p1, const AbstractPreferencesWidget *p2)
{
	return p1->positionHeight() < p2->positionHeight();
}

PreferencesDialog::PreferencesDialog()
{
	//FIXME: This looks wrong.
	//QSettings s;
	//s.beginGroup("GeneralSettings");
	//s.setValue("default_directory", system_default_directory());
	//s.endGroup();

	setWindowIcon(QIcon(":subsurface-icon"));
	setWindowTitle(tr("Preferences"));
	pagesList = new QListWidget();
	pagesStack = new QStackedWidget();
	buttonBox = new QDialogButtonBox(
		QDialogButtonBox::Save |
		QDialogButtonBox::Apply |
		QDialogButtonBox::Cancel);

	pagesList->setMinimumWidth(140);
	pagesList->setMaximumWidth(140);

	QHBoxLayout *h = new QHBoxLayout();
	h->addWidget(pagesList);
	h->addWidget(pagesStack);
	QVBoxLayout *v = new QVBoxLayout();
	v->addLayout(h);
	v->addWidget(buttonBox);

	setLayout(v);

	pages.push_back(new PreferencesLanguage);
	pages.push_back(new PreferencesGeoreference);
	pages.push_back(new PreferencesDefaults);
	pages.push_back(new PreferencesUnits);
	pages.push_back(new PreferencesGraph);
	pages.push_back(new PreferencesNetwork);
	pages.push_back(new PreferencesCloud);
	pages.push_back(new PreferencesEquipment);
	pages.push_back(new PreferencesMedia);
	pages.push_back(new PreferencesDc);
	pages.push_back(new PreferencesLog);
	pages.push_back(new PreferencesReset);
	std::sort(pages.begin(), pages.end(), abstractpreferenceswidget_lessthan);

	for (AbstractPreferencesWidget *page: pages) {
		QListWidgetItem *item = new QListWidgetItem(page->icon(), page->name());
		pagesList->addItem(item);
		pagesStack->addWidget(page);
		page->refreshSettings();
		connect(page, &AbstractPreferencesWidget::settingsChanged, &diveListNotifier, &DiveListNotifier::settingsChanged);
	}

	connect(pagesList, &QListWidget::currentRowChanged,
		pagesStack, &QStackedWidget::setCurrentIndex);
	connect(buttonBox, &QDialogButtonBox::clicked,
		this, &PreferencesDialog::buttonClicked);
}

PreferencesDialog::~PreferencesDialog()
{
}

void PreferencesDialog::buttonClicked(QAbstractButton* btn)
{
	QDialogButtonBox::ButtonRole role = buttonBox->buttonRole(btn);
	switch(role) {
	case QDialogButtonBox::ApplyRole : applyRequested(false); return;
	case QDialogButtonBox::AcceptRole : applyRequested(true); return;
	case QDialogButtonBox::RejectRole : cancelRequested(); return;
	case QDialogButtonBox::ResetRole : defaultsRequested(); return;
	default: return;
	}
}

void PreferencesDialog::refreshPages()
{
	for (AbstractPreferencesWidget *page: pages)
		page->refreshSettings();
}

void PreferencesDialog::applyRequested(bool closeIt)
{
	for (AbstractPreferencesWidget *page: pages)
		page->syncSettings();
	emit diveListNotifier.settingsChanged();
	if (closeIt)
		accept();
}

void PreferencesDialog::cancelRequested()
{
	refreshPages();
	reject();
}

void PreferencesDialog::defaultsRequested()
{
	copy_prefs(&default_prefs, &prefs);
	refreshPages();
	emit diveListNotifier.settingsChanged();
	accept();
}
