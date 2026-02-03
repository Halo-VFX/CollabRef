#include "SelectionRect.h"

#include <QPainter>
#include <QPen>
#include <QBrush>

SelectionRect::SelectionRect(QGraphicsItem *parent)
    : QGraphicsRectItem(parent)
{
    setZValue(9999);
    hide();
}

void SelectionRect::paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
                          QWidget *widget)
{
    Q_UNUSED(option)
    Q_UNUSED(widget)
    
    // Semi-transparent fill
    painter->setBrush(QColor(42, 130, 218, 30));
    
    // Dashed border
    QPen pen(QColor(42, 130, 218), 1, Qt::DashLine);
    painter->setPen(pen);
    
    painter->drawRect(rect());
}
