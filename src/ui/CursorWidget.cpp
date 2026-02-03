#include "CursorWidget.h"

#include <QPainter>
#include <QPainterPath>

CursorWidget::CursorWidget(const QString &userName, const QColor &color, QWidget *parent)
    : QWidget(parent)
    , m_userName(userName)
    , m_color(color)
{
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setAttribute(Qt::WA_TranslucentBackground);
    setFixedSize(100, 30);
}

CursorWidget::~CursorWidget()
{
}

void CursorWidget::setUserName(const QString &name)
{
    m_userName = name;
    update();
}

void CursorWidget::setColor(const QColor &color)
{
    m_color = color;
    update();
}

void CursorWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    // Draw cursor pointer
    QPainterPath cursorPath;
    cursorPath.moveTo(0, 0);
    cursorPath.lineTo(0, 16);
    cursorPath.lineTo(4, 12);
    cursorPath.lineTo(8, 20);
    cursorPath.lineTo(10, 19);
    cursorPath.lineTo(6, 11);
    cursorPath.lineTo(11, 11);
    cursorPath.closeSubpath();
    
    painter.fillPath(cursorPath, m_color);
    painter.setPen(QPen(m_color.darker(150), 1));
    painter.drawPath(cursorPath);
    
    // Draw name label
    QRect labelRect(14, 8, width() - 16, 20);
    painter.setBrush(m_color);
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(labelRect.adjusted(0, 0, 
        -labelRect.width() + painter.fontMetrics().horizontalAdvance(m_userName) + 10, 0),
        3, 3);
    
    painter.setPen(Qt::white);
    painter.setFont(QFont("Arial", 9));
    painter.drawText(labelRect.adjusted(5, 0, 0, 0), Qt::AlignVCenter, m_userName);
}
