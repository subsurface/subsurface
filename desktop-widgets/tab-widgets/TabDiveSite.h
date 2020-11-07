// SPDX-License-Identifier: GPL-2.0
#ifndef TAB_DIVE_SITE_H
#define TAB_DIVE_SITE_H

#include "TabBase.h"
#include "ui_TabDiveSite.h"

class DiveSiteSortedModel;

class TabDiveSite : public TabBase {
	Q_OBJECT
public:
	TabDiveSite(QWidget *parent = 0);
	void updateData() override;
	void clear() override;
private slots:
	void add();
	void diveSiteAdded(struct dive_site *, int idx);
	void diveSiteChanged(struct dive_site *ds, int field);
	void diveSiteClicked(const QModelIndex &);
	void on_purgeUnused_clicked();
	void on_filterText_textChanged(const QString &text);
	void selectionChanged(const QItemSelection &selected, const QItemSelection &deselected);
private:
	Ui::TabDiveSite ui;
	DiveSiteSortedModel *model;
	QVector<dive_site *> selectedDiveSites();
	void hideEvent(QHideEvent *) override;
	void showEvent(QShowEvent *) override;
};

#endif
