#ifndef TOOLBAR_H
#define TOOLBAR_H

#include <QWidget>

class QPushButton;
class QSlider;
class QLabel;

class ToolBar : public QWidget
{
    Q_OBJECT

public:
    explicit ToolBar(QWidget *parent = nullptr);
    ~ToolBar();

    void setZoomLevel(qreal zoom);

signals:
    void zoomInClicked();
    void zoomOutClicked();
    void fitAllClicked();
    void resetViewClicked();
    void gridToggled(bool enabled);

private:
    void setupUI();
    
    QPushButton *m_zoomInBtn;
    QPushButton *m_zoomOutBtn;
    QPushButton *m_fitBtn;
    QPushButton *m_resetBtn;
    QPushButton *m_gridBtn;
    QLabel *m_zoomLabel;
};

#endif // TOOLBAR_H
