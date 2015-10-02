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
	QAction *acceptAction, *rejectAction;
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
protected:
	virtual void currentChanged(const QModelIndex& current, const QModelIndex& previous);
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
	uint32_t currDiveSiteUuid() const;
	void fixPopupPosition();
	void setCurrentDiveSiteUuid(uint32_t uuid);

signals:
	void diveSiteSelected(uint32_t uuid);
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
	uint32_t currUuid;
};

#endif
