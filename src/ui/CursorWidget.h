#ifndef CURSORWIDGET_H
#define CURSORWIDGET_H

#include <QWidget>
#include <QColor>

class CursorWidget : public QWidget
{
    Q_OBJECT

public:
    explicit CursorWidget(const QString &userName, const QColor &color,
                         QWidget *parent = nullptr);
    ~CursorWidget();

    void setUserName(const QString &name);
    void setColor(const QColor &color);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QString m_userName;
    QColor m_color;
};

#endif // CURSORWIDGET_H
