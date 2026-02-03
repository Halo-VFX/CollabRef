#ifndef TEXTITEM_H
#define TEXTITEM_H

#include <QGraphicsTextItem>
#include <QFont>
#include <QColor>

class TextItem : public QGraphicsTextItem
{
    Q_OBJECT

public:
    explicit TextItem(const QString &id, const QString &text = "Double-click to edit",
                      QGraphicsItem *parent = nullptr);
    ~TextItem();

    QString id() const { return m_id; }
    
    void setText(const QString &text);
    QString text() const;
    
    void setTextFont(const QFont &font);
    QFont textFont() const;
    
    void setTextColor(const QColor &color);
    QColor textColor() const;
    
    void setBackgroundColor(const QColor &color);
    QColor backgroundColor() const { return m_backgroundColor; }
    
    void setEditing(bool editing);
    bool isEditing() const { return m_isEditing; }

    // Serialization
    QJsonObject toJson() const;
    static TextItem *fromJson(const QJsonObject &json);

signals:
    void textChanged(TextItem *item);
    void editingFinished(TextItem *item);

protected:
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, 
               QWidget *widget) override;
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;

private:
    QString m_id;
    QColor m_backgroundColor;
    bool m_isEditing;
};

#endif // TEXTITEM_H
