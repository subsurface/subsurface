#include "preferencesdialog.h"

#include "abstractpreferenceswidget.h"
#include "preferences_language.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QListWidget>
#include <QStackedWidget>
#include <QDialogButtonBox>

PreferencesDialogV2::PreferencesDialogV2()
{
	pagesList = new QListWidget();
	pagesStack = new QStackedWidget();
	buttonBox = new QDialogButtonBox(QDialogButtonBox::Apply|QDialogButtonBox::RestoreDefaults|QDialogButtonBox::Cancel);

	pagesList->setMinimumWidth(120);
	pagesList->setMaximumWidth(120);

	QHBoxLayout *h = new QHBoxLayout();
	h->addWidget(pagesList);
	h->addWidget(pagesStack);

	QVBoxLayout *v = new QVBoxLayout();
	v->addLayout(h);
	v->addWidget(buttonBox);

	setLayout(v);
	
	addPreferencePage(new PreferencesLanguage());
	refreshPages();
	connect(pagesList, &QListWidget::currentRowChanged,
		pagesStack, &QStackedWidget::setCurrentIndex);
}

PreferencesDialogV2::~PreferencesDialogV2()
{
}

bool abstractpreferenceswidget_lessthan(AbstractPreferencesWidget *p1, AbstractPreferencesWidget *p2)
{
	return p1->positionHeight() <= p2->positionHeight();
}

void PreferencesDialogV2::addPreferencePage(AbstractPreferencesWidget *page)
{
	pages.push_back(page);
	qSort(pages.begin(), pages.end(), abstractpreferenceswidget_lessthan);
}

void PreferencesDialogV2::refreshPages()
{
	// Remove things
	pagesList->clear();
	while(pagesStack->count()) {
		QWidget *curr = pagesStack->widget(0);
		pagesStack->removeWidget(curr);
		curr->setParent(0);
	}

	// Readd things.
	Q_FOREACH(AbstractPreferencesWidget *page, pages) {
		QListWidgetItem *item = new QListWidgetItem(page->icon(), page->name());
		pagesList->addItem(item);
		pagesStack->addWidget(page);
	}
}

void PreferencesDialogV2::applyRequested()
{
	//TODO
}

void PreferencesDialogV2::cancelRequested()
{
	//TODO
}

void PreferencesDialogV2::defaultsRequested()
{
	//TODO
}
