// SPDX-License-Identifier: GPL-2.0
#ifndef LOCATIONINFORMATION_H
#define LOCATIONINFORMATION_H

#include "core/units.h"
#include "ui_locationinformation.h"
#include "modeldelegates.h"
#include "qt-models/divelocationmodel.h"
#include <stdint.h>
#include <QAbstractListModel>
#include <QSortFilterProxyModel>

struct dive_site;

class LocationInformationWidget : public QGroupBox {
	Q_OBJECT
public:
	LocationInformationWidget(QWidget *parent = 0);
	bool eventFilter(QObject*, QEvent*) override;
	void initFields(dive_site *ds);
	Ui::LocationInformation ui;

protected:
	void enableLocationButtons(bool enable);

public slots:
	void acceptChanges();
	void on_diveSiteCountry_editingFinished();
	void on_diveSiteCoordinates_editingFinished();
	void on_diveSiteCoordinates_textEdited(const QString &s);
	void on_diveSiteDescription_editingFinished();
	void on_diveSiteName_editingFinished();
	void on_diveSiteNotes_editingFinished();
	void on_diveSiteDistance_textChanged(const QString &s);
	void reverseGeocode();
	void mergeSelectedDiveSites();
	void on_GPSbutton_clicked();
private slots:
	void updateLabels();
	void diveSiteChanged(struct dive_site *ds, int field);
	void diveSiteDeleted(struct dive_site *ds, int);
	void unitsChanged();
private:
	void keyPressEvent(QKeyEvent *e) override;
	void clearLabels();
	void coordinatesSetWarning(bool warn);
	GPSLocationInformationModel filter_model;
	dive_site *diveSite;
	int64_t closeDistance; // Distance of "close" dive sites in mm
};

class DiveLocationFilterProxyModel : public QSortFilterProxyModel {
	Q_OBJECT
	QString filter;
public:
	DiveLocationFilterProxyModel(QObject *parent = 0);
	bool filterAcceptsRow(int source_row, const QModelIndex& source_parent) const override;
	bool lessThan(const QModelIndex& source_left, const QModelIndex& source_right) const override;
	void setFilter(const QString &filter);
	void setCurrentLocation(location_t loc);
private:
	location_t currentLocation; // Sort by distance to that location
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
	DiveLocationLineEdit(QWidget *parent =0 );
	void refreshDiveSiteCache();
	void setTemporaryDiveSiteName(const QString& s);
	bool eventFilter(QObject*, QEvent*);
	void itemActivated(const QModelIndex& index);
	struct dive_site *currDiveSite() const;
	void fixPopupPosition();
	void setCurrentDiveSite(struct dive *d);
	void showAllSites();

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
	LocationFilterDelegate delegate;
	struct dive_site *currDs;
};

#endif
