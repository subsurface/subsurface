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
	void setCurrentDiveSiteByUuid(uint32_t uuid);
	void updateGpsCoordinates(void);
	void markChangedWidget(QWidget *w);
	void enableEdition();
	void resetState();
	void resetPallete();
	void on_diveSiteCoordinates_textChanged(const QString& text);
	void on_diveSiteDescription_textChanged(const QString& text);
	void on_diveSiteName_textChanged(const QString& text);
	void on_diveSiteNotes_textChanged();
signals:
	void informationManagementEnded();
	void coordinatesChanged();
	void startFilterDiveSite(uint32_t uuid);
	void stopFilterDiveSite();
private:
	struct dive_site *currentDs;
	Ui::LocationInformation ui;
	bool modified;
	QAction *closeAction, *acceptAction, *rejectAction;
};

#endif
