#ifndef LOCATIONINFORMATION_H
#define LOCATIONINFORMATION_H

#include "ui_locationInformation.h"
#include <stdint.h>
#include <QAbstractListModel>

class LocationInformationModel : public QAbstractListModel {
Q_OBJECT
public:
	LocationInformationModel(QObject *obj = 0);
	int rowCount(const QModelIndex &parent = QModelIndex()) const;
	QVariant data(const QModelIndex &index = QModelIndex(), int role = Qt::DisplayRole) const;
	void update();
private:
	int internalRowCount;
};

class LocationInformationWidget : public QGroupBox {
Q_OBJECT
public:
	LocationInformationWidget(QWidget *parent = 0);
protected:
	void showEvent(QShowEvent *);
\
public slots:
	void acceptChanges();
	void rejectChanges();
	void setLocationId(uint32_t uuid);
	void updateGpsCoordinates(void);
	void markChangedWidget(QWidget *w);
	void enableEdition();
	void resetState();
	void resetPallete();
	void setCurrentDiveSite(int dive_nr);
	void on_diveSiteCoordinates_textChanged(const QString& text);
	void on_diveSiteDescription_textChanged(const QString& text);
	void on_diveSiteName_textChanged(const QString& text);
	void on_diveSiteNotes_textChanged();
signals:
	void informationManagementEnded();

private:
	struct dive_site *currentDs;
	Ui::LocationInformation ui;
	bool modified;
	QAction *closeAction, *acceptAction, *rejectAction;
};

#endif
