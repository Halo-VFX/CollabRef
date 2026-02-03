#include "ToolBar.h"

#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QSlider>

ToolBar::ToolBar(QWidget *parent)
    : QWidget(parent)
{
    setupUI();
}

ToolBar::~ToolBar()
{
}

void ToolBar::setupUI()
{
    setFixedHeight(28);
    setStyleSheet("background: #252526;");
    
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(8, 2, 8, 2);
    layout->setSpacing(4);
    
    QString buttonStyle = R"(
        QPushButton {
            background: transparent;
            border: 1px solid transparent;
            border-radius: 3px;
            color: #ccc;
            font-size: 11px;
            padding: 2px 8px;
            min-width: 24px;
        }
        QPushButton:hover {
            background: #3e3e42;
            border-color: #555;
        }
        QPushButton:pressed {
            background: #2a82da;
        }
        QPushButton:checked {
            background: #2a82da;
            color: white;
        }
    )";
    
    // Zoom controls
    m_zoomOutBtn = new QPushButton("−", this);
    m_zoomOutBtn->setStyleSheet(buttonStyle);
    m_zoomOutBtn->setToolTip("Zoom Out");
    connect(m_zoomOutBtn, &QPushButton::clicked, this, &ToolBar::zoomOutClicked);
    layout->addWidget(m_zoomOutBtn);
    
    m_zoomLabel = new QLabel("100%", this);
    m_zoomLabel->setStyleSheet("color: #aaa; font-size: 11px; min-width: 45px;");
    m_zoomLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(m_zoomLabel);
    
    m_zoomInBtn = new QPushButton("+", this);
    m_zoomInBtn->setStyleSheet(buttonStyle);
    m_zoomInBtn->setToolTip("Zoom In");
    connect(m_zoomInBtn, &QPushButton::clicked, this, &ToolBar::zoomInClicked);
    layout->addWidget(m_zoomInBtn);
    
    layout->addSpacing(16);
    
    // View controls
    m_fitBtn = new QPushButton("Fit", this);
    m_fitBtn->setStyleSheet(buttonStyle);
    m_fitBtn->setToolTip("Fit All (F)");
    connect(m_fitBtn, &QPushButton::clicked, this, &ToolBar::fitAllClicked);
    layout->addWidget(m_fitBtn);
    
    m_resetBtn = new QPushButton("Reset", this);
    m_resetBtn->setStyleSheet(buttonStyle);
    m_resetBtn->setToolTip("Reset View (R)");
    connect(m_resetBtn, &QPushButton::clicked, this, &ToolBar::resetViewClicked);
    layout->addWidget(m_resetBtn);
    
    layout->addSpacing(16);
    
    // Grid toggle
    m_gridBtn = new QPushButton("Grid", this);
    m_gridBtn->setStyleSheet(buttonStyle);
    m_gridBtn->setCheckable(true);
    m_gridBtn->setToolTip("Toggle Grid");
    connect(m_gridBtn, &QPushButton::toggled, this, &ToolBar::gridToggled);
    layout->addWidget(m_gridBtn);
    
    layout->addStretch();
    
    // Help text
    QLabel *helpLabel = new QLabel("Space+Drag: Pan | Scroll: Zoom | Drop images to add", this);
    helpLabel->setStyleSheet("color: #666; font-size: 10px;");
    layout->addWidget(helpLabel);
}

void ToolBar::setZoomLevel(qreal zoom)
{
    int percent = qRound(zoom * 100);
    m_zoomLabel->setText(QString("%1%").arg(percent));
}
