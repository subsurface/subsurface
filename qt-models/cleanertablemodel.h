#ifndef CLEANERTABLEMODEL_H
#define CLEANERTABLEMODEL_H

#include <QAbstractTableModel>
#include <QStringList>

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

private:
	QStringList headers;
};

#endif
