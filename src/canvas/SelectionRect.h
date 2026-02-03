#ifndef SELECTIONRECT_H
#define SELECTIONRECT_H

#include <QGraphicsRectItem>

class SelectionRect : public QGraphicsRectItem
{
public:
    SelectionRect(QGraphicsItem *parent = nullptr);
    
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
               QWidget *widget) override;
};

#endif // SELECTIONRECT_H
