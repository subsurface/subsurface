#ifndef MODELDELEGATES_H
#define MODELDELEGATES_H

#include <QStyledItemDelegate>

class StarWidgetsDelegate : public QStyledItemDelegate {
	Q_OBJECT
public:
    explicit StarWidgetsDelegate(QWidget* parent = 0);
    virtual void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const;
    virtual QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const;
private:
	QWidget *parentWidget;
};
#endif
