#ifndef CANVASVIEW_H
#define CANVASVIEW_H

#include <QGraphicsView>
#include <QPointF>
#include <QSize>

class CanvasScene;

class CanvasView : public QGraphicsView
{
    Q_OBJECT

public:
    explicit CanvasView(CanvasScene *scene, QWidget *parent = nullptr);
    ~CanvasView();

    qreal currentZoom() const { return m_currentZoom; }
    bool isScaleWithWindow() const { return m_scaleWithWindow; }
    
public slots:
    void zoomIn();
    void zoomOut();
    void resetZoom();
    void resetView();
    void fitAll();
    void setZoom(qreal zoom);
    void setScaleWithWindow(bool enabled);
    void toggleScaleWithWindow();
    void setGridVisible(bool visible);
    bool isGridVisible() const { return m_showGrid; }

signals:
    void zoomChanged(qreal zoom);
    void viewportMoved(QPointF center);
    void scaleWithWindowChanged(bool enabled);

protected:
    void wheelEvent(QWheelEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    void drawBackground(QPainter *painter, const QRectF &rect) override;
    void resizeEvent(QResizeEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private:
    void applyZoom(qreal factor, QPointF centerPoint);
    void updateCursor();

    CanvasScene *m_scene;
    qreal m_currentZoom;
    qreal m_minZoom;
    qreal m_maxZoom;
    
    bool m_isPanning;
    bool m_isSpacePressed;
    QPoint m_lastPanPoint;
    
    // Scale with window
    bool m_scaleWithWindow;
    QSize m_lastViewportSize;
    
    // Grid settings
    bool m_showGrid;
    int m_gridSize;
    QColor m_gridColor;
    QColor m_backgroundColor;
};

#endif // CANVASVIEW_H
