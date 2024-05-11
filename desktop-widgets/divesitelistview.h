// SPDX-License-Identifier: GPL-2.0
#ifndef DIVE_SITE_LIST_VIEW_H
#define DIVE_SITE_LIST_VIEW_H

#include "ui_divesitelistview.h"

class DiveSiteSortedModel;

class DiveSiteListView : public QWidget {
	Q_OBJECT
public:
	DiveSiteListView(QWidget *parent = 0);
private slots:
	void add();
	void done();
	void diveSiteAdded(struct dive_site *, int idx);
	void diveSiteChanged(struct dive_site *ds, int field);
	void diveSiteClicked(const QModelIndex &);
	void on_purgeUnused_clicked();
	void on_filterText_textChanged(const QString &text);
	void selectionChanged(const QItemSelection &selected, const QItemSelection &deselected);
private:
	Ui::DiveSiteListView ui;
	DiveSiteSortedModel *model;
	std::vector<dive_site *> selectedDiveSites();
	void hideEvent(QHideEvent *) override;
	void showEvent(QShowEvent *) override;
};

#endif
