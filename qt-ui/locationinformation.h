#ifndef LOCATIONINFORMATION_H
#define LOCATIONINFORMATION_H

#include "ui_locationInformation.h"
#include <stdint.h>
#include <QAbstractListModel>
#include <QSortFilterProxyModel>

class LocationInformationWidget : public QGroupBox {
Q_OBJECT
public:
	LocationInformationWidget(QWidget *parent = 0);
	virtual bool eventFilter(QObject*, QEvent*);

protected:
	void showEvent(QShowEvent *);

public slots:
	void acceptChanges();
	void rejectChanges();
	void updateGpsCoordinates();
	void markChangedWidget(QWidget *w);
	void enableEdition();
	void resetState();
	void resetPallete();
	void on_diveSiteCoordinates_textChanged(const QString& text);
	void on_diveSiteDescription_textChanged(const QString& text);
	void on_diveSiteName_textChanged(const QString& text);
	void on_diveSiteNotes_textChanged();
	void reverseGeocode();
	void mergeSelectedDiveSites();
private slots:
	void updateLabels();
signals:
	void startEditDiveSite(uint32_t uuid);
	void endEditDiveSite();
	void coordinatesChanged();
	void startFilterDiveSite(uint32_t uuid);
	void stopFilterDiveSite();
	void requestCoordinates();
	void endRequestCoordinates();

private:
	Ui::LocationInformation ui;
	bool modified;
	QAction *closeAction, *acceptAction, *rejectAction;
};

class LocationManagementEditHelper : public QObject {
Q_OBJECT
public:
	bool eventFilter(QObject *obj, QEvent *ev);
	void handleActivation(const QModelIndex& activated);
	void resetDiveSiteUuid();
	uint32_t diveSiteUuid() const;
signals:
	void setLineEditText(const QString& text);
private:
	uint32_t last_uuid;
};

class DiveLocationFilterProxyModel : public QSortFilterProxyModel {
	Q_OBJECT
public:
	DiveLocationFilterProxyModel(QObject *parent = 0);
	virtual bool filterAcceptsRow(int source_row, const QModelIndex& source_parent) const;
	virtual bool lessThan(const QModelIndex& source_left, const QModelIndex& source_right) const;
};

class DiveLocationModel : public QAbstractTableModel {
	Q_OBJECT
public:
	enum columns{UUID, NAME, LATITUDE, LONGITUDE, DESCRIPTION, NOTES, COLUMNS};
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
};

class DiveLocationLineEdit : public QLineEdit {
	Q_OBJECT
public:
	DiveLocationLineEdit(QWidget *parent =0 );
	void refreshDiveSiteCache();
	void setTemporaryDiveSiteName(const QString& s);
protected:
	void keyPressEvent(QKeyEvent *ev);
	void showPopup();
private:
	DiveLocationFilterProxyModel *proxy;
	DiveLocationModel *model;
	DiveLocationListView *view;
};

#endif
