#ifndef MODELDELEGATES_H
#define MODELDELEGATES_H

#include <QAbstractItemDelegate>

class StarWidgetsDelegate : public QAbstractItemDelegate {
	Q_OBJECT
public:
    virtual void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const;
    virtual QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const;
};
#endif
