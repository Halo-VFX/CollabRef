#include "TitleBar.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QMouseEvent>
#include <QPainter>
#include <QTimer>
#include <QApplication>

TitleBar::TitleBar(QWidget *parent)
    : QWidget(parent)
    , m_isDragging(false)
    , m_notificationTimer(new QTimer(this))
{
    setupUI();
    
    m_notificationTimer->setSingleShot(true);
    connect(m_notificationTimer, &QTimer::timeout, this, [this]() {
        m_notificationLabel->hide();
    });
}

TitleBar::~TitleBar()
{
}

void TitleBar::setupUI()
{
    setFixedHeight(32);
    setAutoFillBackground(true);
    
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(8, 0, 0, 0);
    layout->setSpacing(0);
    
    // App icon/logo area (could add icon here)
    QLabel *iconLabel = new QLabel("◆", this);
    iconLabel->setStyleSheet("color: #2a82da; font-size: 14px;");
    layout->addWidget(iconLabel);
    
    layout->addSpacing(8);
    
    // Title
    m_titleLabel = new QLabel("CollabRef", this);
    m_titleLabel->setStyleSheet("color: white; font-size: 12px;");
    layout->addWidget(m_titleLabel);
    
    // Connection indicator
    m_connectionIndicator = new QLabel("●", this);
    m_connectionIndicator->setStyleSheet("color: #555; font-size: 10px; margin-left: 8px;");
    m_connectionIndicator->setToolTip("Not connected");
    layout->addWidget(m_connectionIndicator);
    
    layout->addSpacing(16);
    
    // Notification label
    m_notificationLabel = new QLabel(this);
    m_notificationLabel->setStyleSheet(
        "color: #aaa; font-size: 11px; background: transparent;");
    m_notificationLabel->hide();
    layout->addWidget(m_notificationLabel);
    
    layout->addStretch();
    
    // Window buttons
    QString buttonStyle = R"(
        QPushButton {
            background: transparent;
            border: none;
            color: #aaa;
            font-size: 10px;
            padding: 0 16px;
            min-height: 32px;
        }
        QPushButton:hover {
            background: #444;
        }
    )";
    
    QString closeStyle = R"(
        QPushButton {
            background: transparent;
            border: none;
            color: #aaa;
            font-size: 10px;
            padding: 0 16px;
            min-height: 32px;
        }
        QPushButton:hover {
            background: #e81123;
            color: white;
        }
    )";
    
    m_minimizeBtn = new QPushButton("─", this);
    m_minimizeBtn->setStyleSheet(buttonStyle);
    m_minimizeBtn->setFixedSize(46, 32);
    connect(m_minimizeBtn, &QPushButton::clicked, this, &TitleBar::minimizeClicked);
    layout->addWidget(m_minimizeBtn);
    
    m_maximizeBtn = new QPushButton("□", this);
    m_maximizeBtn->setStyleSheet(buttonStyle);
    m_maximizeBtn->setFixedSize(46, 32);
    connect(m_maximizeBtn, &QPushButton::clicked, this, &TitleBar::maximizeClicked);
    layout->addWidget(m_maximizeBtn);
    
    m_closeBtn = new QPushButton("✕", this);
    m_closeBtn->setStyleSheet(closeStyle);
    m_closeBtn->setFixedSize(46, 32);
    connect(m_closeBtn, &QPushButton::clicked, this, &TitleBar::closeClicked);
    layout->addWidget(m_closeBtn);
}

void TitleBar::setTitle(const QString &title)
{
    m_titleLabel->setText(title);
}

void TitleBar::setConnectionStatus(bool connected)
{
    if (connected) {
        m_connectionIndicator->setStyleSheet("color: #2ecc71; font-size: 10px; margin-left: 8px;");
        m_connectionIndicator->setToolTip("Connected");
    } else {
        m_connectionIndicator->setStyleSheet("color: #555; font-size: 10px; margin-left: 8px;");
        m_connectionIndicator->setToolTip("Not connected");
    }
}

void TitleBar::showNotification(const QString &text, int durationMs)
{
    m_notificationLabel->setText(text);
    m_notificationLabel->show();
    m_notificationTimer->start(durationMs);
}

void TitleBar::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_isDragging = true;
        m_dragPosition = event->globalPos() - window()->frameGeometry().topLeft();
        event->accept();
    }
}

void TitleBar::mouseMoveEvent(QMouseEvent *event)
{
    if (m_isDragging && (event->buttons() & Qt::LeftButton)) {
        window()->move(event->globalPos() - m_dragPosition);
        event->accept();
    }
}

void TitleBar::mouseReleaseEvent(QMouseEvent *event)
{
    m_isDragging = false;
    QWidget::mouseReleaseEvent(event);
}

void TitleBar::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        emit maximizeClicked();
    }
}

void TitleBar::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    
    QPainter painter(this);
    painter.fillRect(rect(), QColor(30, 30, 30));
    
    // Bottom border
    painter.setPen(QColor(50, 50, 50));
    painter.drawLine(0, height() - 1, width(), height() - 1);
}
