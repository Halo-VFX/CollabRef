#include "TextItem.h"

#include <QPainter>
#include <QGraphicsSceneMouseEvent>
#include <QFocusEvent>
#include <QKeyEvent>
#include <QTextCursor>
#include <QJsonObject>

TextItem::TextItem(const QString &id, const QString &text, QGraphicsItem *parent)
    : QGraphicsTextItem(parent)
    , m_id(id)
    , m_backgroundColor(QColor(50, 50, 55, 200))
    , m_isEditing(false)
{
    setPlainText(text);
    setDefaultTextColor(Qt::white);
    
    QFont font("Arial", 14);
    setFont(font);
    
    setFlags(QGraphicsItem::ItemIsSelectable |
             QGraphicsItem::ItemIsMovable |
             QGraphicsItem::ItemSendsGeometryChanges);
    
    setTextInteractionFlags(Qt::NoTextInteraction);
    
    // Set a minimum width
    setTextWidth(200);
}

TextItem::~TextItem()
{
}

void TextItem::setText(const QString &text)
{
    setPlainText(text);
}

QString TextItem::text() const
{
    return toPlainText();
}

void TextItem::setTextFont(const QFont &font)
{
    setFont(font);
}

QFont TextItem::textFont() const
{
    return font();
}

void TextItem::setTextColor(const QColor &color)
{
    setDefaultTextColor(color);
}

QColor TextItem::textColor() const
{
    return defaultTextColor();
}

void TextItem::setBackgroundColor(const QColor &color)
{
    m_backgroundColor = color;
    update();
}

void TextItem::setEditing(bool editing)
{
    m_isEditing = editing;
    if (editing) {
        setTextInteractionFlags(Qt::TextEditorInteraction);
        setFocus();
        QTextCursor cursor = textCursor();
        cursor.select(QTextCursor::Document);
        setTextCursor(cursor);
    } else {
        setTextInteractionFlags(Qt::NoTextInteraction);
        QTextCursor cursor = textCursor();
        cursor.clearSelection();
        setTextCursor(cursor);
    }
    update();
}

void TextItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, 
                     QWidget *widget)
{
    // Draw background
    painter->setBrush(m_backgroundColor);
    painter->setPen(Qt::NoPen);
    QRectF rect = boundingRect();
    rect.adjust(-5, -5, 5, 5);
    painter->drawRoundedRect(rect, 5, 5);
    
    // Draw selection indicator
    if (isSelected()) {
        painter->setPen(QPen(QColor(0, 150, 255), 2));
        painter->setBrush(Qt::NoBrush);
        painter->drawRoundedRect(rect, 5, 5);
    }
    
    // Draw text
    QGraphicsTextItem::paint(painter, option, widget);
}

void TextItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event)
{
    setEditing(true);
    QGraphicsTextItem::mouseDoubleClickEvent(event);
}

void TextItem::focusOutEvent(QFocusEvent *event)
{
    if (m_isEditing) {
        setEditing(false);
        emit editingFinished(this);
    }
    QGraphicsTextItem::focusOutEvent(event);
}

void TextItem::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        setEditing(false);
        emit editingFinished(this);
        return;
    }
    
    QGraphicsTextItem::keyPressEvent(event);
    emit textChanged(this);
}

QVariant TextItem::itemChange(GraphicsItemChange change, const QVariant &value)
{
    if (change == ItemPositionHasChanged || change == ItemRotationHasChanged) {
        emit textChanged(this);
    }
    return QGraphicsTextItem::itemChange(change, value);
}

QJsonObject TextItem::toJson() const
{
    QJsonObject json;
    json["id"] = m_id;
    json["type"] = "text";
    json["text"] = toPlainText();
    json["x"] = pos().x();
    json["y"] = pos().y();
    json["rotation"] = rotation();
    json["fontFamily"] = font().family();
    json["fontSize"] = font().pointSize();
    json["textColor"] = defaultTextColor().name();
    json["bgColor"] = m_backgroundColor.name(QColor::HexArgb);
    return json;
}

TextItem *TextItem::fromJson(const QJsonObject &json)
{
    QString id = json["id"].toString();
    QString text = json["text"].toString();
    
    TextItem *item = new TextItem(id, text);
    
    item->setPos(json["x"].toDouble(), json["y"].toDouble());
    item->setRotation(json["rotation"].toDouble());
    
    if (json.contains("fontFamily")) {
        QFont font(json["fontFamily"].toString(), json["fontSize"].toInt(14));
        item->setFont(font);
    }
    
    if (json.contains("textColor")) {
        item->setDefaultTextColor(QColor(json["textColor"].toString()));
    }
    
    if (json.contains("bgColor")) {
        item->setBackgroundColor(QColor(json["bgColor"].toString()));
    }
    
    return item;
}
