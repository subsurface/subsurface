// SPDX-License-Identifier: GPL-2.0
#ifndef LOCATIONINFORMATION_H
#define LOCATIONINFORMATION_H

#include "core/units.h"
#include "core/divesite.h"
#include "core/taxonomy.h"
#include "ui_locationinformation.h"
#include "qt-models/divelocationmodel.h"
#include <stdint.h>
#include <QAbstractListModel>
#include <QSortFilterProxyModel>

class LocationInformationWidget : public QGroupBox {
	Q_OBJECT
public:
	LocationInformationWidget(QWidget *parent = 0);
	bool eventFilter(QObject*, QEvent*) override;
	void initFields(dive_site *ds);

protected:
	void enableLocationButtons(bool enable);

public slots:
	void acceptChanges();
	void rejectChanges();
	void updateGpsCoordinates(const location_t &);
	void markChangedWidget(QWidget *w);
	void enableEdition();
	void resetState();
	void resetPallete();
	void on_diveSiteCountry_textChanged(const QString& text);
	void on_diveSiteCoordinates_textChanged(const QString& text);
	void on_diveSiteDescription_editingFinished();
	void on_diveSiteName_editingFinished();
	void on_diveSiteNotes_textChanged();
	void reverseGeocode();
	void mergeSelectedDiveSites();
private slots:
	void updateLabels();
	void updateLocationOnMap();
	void diveSiteChanged(struct dive_site *ds, int field);
signals:
	void endEditDiveSite();

private:
	void clearLabels();
	Ui::LocationInformation ui;
	bool modified;
	QAction *acceptAction, *rejectAction;
	GPSLocationInformationModel filter_model;
	dive_site *diveSite;
	taxonomy_data taxonomy;
};

class DiveLocationFilterProxyModel : public QSortFilterProxyModel {
	Q_OBJECT
public:
	DiveLocationFilterProxyModel(QObject *parent = 0);
	bool filterAcceptsRow(int source_row, const QModelIndex& source_parent) const override;
	bool lessThan(const QModelIndex& source_left, const QModelIndex& source_right) const override;
};

class DiveLocationModel : public QAbstractTableModel {
	Q_OBJECT
public:
	DiveLocationModel(QObject *o = 0);
	void resetModel();
	QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
	int rowCount(const QModelIndex& parent = QModelIndex()) const;
	int columnCount(const QModelIndex& parent = QModelIndex()) const;
	bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole);
private:
	QString new_ds_value[2];
};

class DiveLocationListView : public QListView {
	Q_OBJECT
public:
	DiveLocationListView(QWidget *parent = 0);
protected:
	void currentChanged(const QModelIndex& current, const QModelIndex& previous) override;
signals:
	void currentIndexChanged(const QModelIndex& current);
};

class DiveLocationLineEdit : public QLineEdit {
	Q_OBJECT
public:
	enum DiveSiteType { NO_DIVE_SITE, NEW_DIVE_SITE, EXISTING_DIVE_SITE };
	DiveLocationLineEdit(QWidget *parent =0 );
	void refreshDiveSiteCache();
	void setTemporaryDiveSiteName(const QString& s);
	bool eventFilter(QObject*, QEvent*);
	void itemActivated(const QModelIndex& index);
	DiveSiteType currDiveSiteType() const;
	struct dive_site *currDiveSite() const;
	void fixPopupPosition();
	void setCurrentDiveSite(struct dive_site *ds);

signals:
	void diveSiteSelected();
	void entered(const QModelIndex& index);
	void currentChanged(const QModelIndex& index);

protected:
	void keyPressEvent(QKeyEvent *ev);
	void focusOutEvent(QFocusEvent *ev);
	void showPopup();

private:
	using QLineEdit::setText;
	DiveLocationFilterProxyModel *proxy;
	DiveLocationModel *model;
	DiveLocationListView *view;
	DiveSiteType currType;
	struct dive_site *currDs;
};

#endif
