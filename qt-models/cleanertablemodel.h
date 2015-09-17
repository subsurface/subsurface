#ifndef CLEANERTABLEMODEL_H
#define CLEANERTABLEMODEL_H

#include <QAbstractTableModel>
#include <QStringList>
#include <QPixmap>

/* Retrieve the trash icon pixmap, common to most table models */
const QPixmap &trashIcon();
const QPixmap &trashForbiddenIcon();

/* When using a QAbstractTableModel, consider using this instead
 * of the default implementation, as it's easyer to setup the columns
 * and headers.
 * Most subsurface classes uses this one to save loads of lines
 * of code and share a consistent layout. */

class CleanerTableModel : public QAbstractTableModel {
	Q_OBJECT
public:
	explicit CleanerTableModel(QObject *parent = 0);
	virtual int columnCount(const QModelIndex &parent = QModelIndex()) const;
	virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

protected:
	void setHeaderDataStrings(const QStringList &headers);
signals:

	/* instead of using QMessageBox directly, wire a QWidget to this signal and display the result.
	 * This is because the QModels will be used from the Mobile version and the desktop version. */
	void warningMessage(const QString& title, const QString& message);

private:
	QStringList headers;
};

/* Has the string value changed */
#define CHANGED() \
	(vString = value.toString()) != data(index, role).toString()

#endif
