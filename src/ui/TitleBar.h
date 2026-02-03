#ifndef TITLEBAR_H
#define TITLEBAR_H

#include <QWidget>

class QLabel;
class QPushButton;
class QTimer;

class TitleBar : public QWidget
{
    Q_OBJECT

public:
    explicit TitleBar(QWidget *parent = nullptr);
    ~TitleBar();

    void setTitle(const QString &title);
    void setConnectionStatus(bool connected);
    void showNotification(const QString &text, int durationMs = 3000);

signals:
    void minimizeClicked();
    void maximizeClicked();
    void closeClicked();

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:
    void setupUI();
    
    QLabel *m_titleLabel;
    QLabel *m_notificationLabel;
    QLabel *m_connectionIndicator;
    QPushButton *m_minimizeBtn;
    QPushButton *m_maximizeBtn;
    QPushButton *m_closeBtn;
    
    QPoint m_dragPosition;
    bool m_isDragging;
    
    QTimer *m_notificationTimer;
};

#endif // TITLEBAR_H
