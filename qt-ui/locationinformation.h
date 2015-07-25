#ifndef LOCATIONINFORMATION_H
#define LOCATIONINFORMATION_H

#include "ui_locationInformation.h"
#include <stdint.h>
#include <QAbstractListModel>

class LocationInformationWidget : public QGroupBox {
Q_OBJECT
public:
	LocationInformationWidget(QWidget *parent = 0);
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
private slots:
	void setCurrentDiveSiteByUuid(uint32_t uuid);
signals:
	void startEditDiveSite(uint32_t uuid);
	void endEditDiveSite();
	void informationManagementEnded();
	void coordinatesChanged();
	void startFilterDiveSite(uint32_t uuid);
	void stopFilterDiveSite();
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
#endif
