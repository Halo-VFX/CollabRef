#include "CanvasView.h"
#include "CanvasScene.h"
#include "ImageItem.h"

#include <QWheelEvent>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QResizeEvent>
#include <QPainter>
#include <QScrollBar>
#include <QtMath>
#include <QApplication>
#include <QMimeData>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QUrl>

CanvasView::CanvasView(CanvasScene *scene, QWidget *parent)
    : QGraphicsView(scene, parent)
    , m_scene(scene)
    , m_currentZoom(1.0)
    , m_minZoom(0.01)
    , m_maxZoom(50.0)
    , m_isPanning(false)
    , m_isSpacePressed(false)
    , m_scaleWithWindow(false)
    , m_showGrid(false)
    , m_gridSize(50)
    , m_gridColor(60, 60, 60)
    , m_backgroundColor(35, 35, 38)
{
    setRenderHints(QPainter::Antialiasing | 
                   QPainter::SmoothPixmapTransform |
                   QPainter::TextAntialiasing);
    
    setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setResizeAnchor(QGraphicsView::AnchorViewCenter);
    
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    
    setDragMode(QGraphicsView::NoDrag);
    
    // Large scene rect for infinite canvas feel
    setSceneRect(-50000, -50000, 100000, 100000);
    
    // Enable caching
    setCacheMode(QGraphicsView::CacheBackground);
    
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
    
    // Enable drag and drop
    setAcceptDrops(true);
    viewport()->setAcceptDrops(true);
    
    // Background
    setBackgroundBrush(m_backgroundColor);
    
    // Initialize last viewport size
    m_lastViewportSize = viewport()->size();
}

CanvasView::~CanvasView()
{
}

void CanvasView::zoomIn()
{
    applyZoom(1.25, viewport()->rect().center());
}

void CanvasView::zoomOut()
{
    applyZoom(0.8, viewport()->rect().center());
}

void CanvasView::resetZoom()
{
    setZoom(1.0);
}

void CanvasView::resetView()
{
    resetTransform();
    m_currentZoom = 1.0;
    centerOn(0, 0);
    emit zoomChanged(m_currentZoom);
}

void CanvasView::fitAll()
{
    QRectF itemsBounds = m_scene->itemsBoundingRect();
    if (itemsBounds.isEmpty()) {
        resetView();
        return;
    }
    
    // Add padding
    itemsBounds.adjust(-50, -50, 50, 50);
    
    fitInView(itemsBounds, Qt::KeepAspectRatio);
    
    // Update current zoom based on transform
    m_currentZoom = transform().m11();
    emit zoomChanged(m_currentZoom);
}

void CanvasView::setZoom(qreal zoom)
{
    zoom = qBound(m_minZoom, zoom, m_maxZoom);
    
    qreal factor = zoom / m_currentZoom;
    m_currentZoom = zoom;
    
    scale(factor, factor);
    emit zoomChanged(m_currentZoom);
}

void CanvasView::applyZoom(qreal factor, QPointF centerPoint)
{
    qreal newZoom = m_currentZoom * factor;
    newZoom = qBound(m_minZoom, newZoom, m_maxZoom);
    
    if (qFuzzyCompare(newZoom, m_currentZoom)) {
        return;
    }
    
    // Get scene position before zoom
    QPointF scenePos = mapToScene(centerPoint.toPoint());
    
    // Apply zoom
    qreal actualFactor = newZoom / m_currentZoom;
    m_currentZoom = newZoom;
    scale(actualFactor, actualFactor);
    
    // Adjust to keep point under cursor
    QPointF newScenePos = mapToScene(centerPoint.toPoint());
    QPointF delta = scenePos - newScenePos;
    translate(-delta.x(), -delta.y());
    
    emit zoomChanged(m_currentZoom);
}

void CanvasView::wheelEvent(QWheelEvent *event)
{
    // Zoom with wheel
    qreal factor = 1.0;
    
    if (event->angleDelta().y() > 0) {
        factor = 1.1;
    } else if (event->angleDelta().y() < 0) {
        factor = 0.9;
    }
    
    // Slower zoom with Ctrl
    if (event->modifiers() & Qt::ControlModifier) {
        factor = 1.0 + (factor - 1.0) * 0.5;
    }
    
    applyZoom(factor, event->pos());
    event->accept();
}

void CanvasView::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::MiddleButton || 
        (event->button() == Qt::LeftButton && m_isSpacePressed)) {
        m_isPanning = true;
        m_lastPanPoint = event->pos();
        setCursor(Qt::ClosedHandCursor);
        event->accept();
        return;
    }
    
    QGraphicsView::mousePressEvent(event);
}

void CanvasView::mouseMoveEvent(QMouseEvent *event)
{
    if (m_isPanning) {
        QPoint delta = event->pos() - m_lastPanPoint;
        m_lastPanPoint = event->pos();
        
        horizontalScrollBar()->setValue(horizontalScrollBar()->value() - delta.x());
        verticalScrollBar()->setValue(verticalScrollBar()->value() - delta.y());
        
        emit viewportMoved(mapToScene(viewport()->rect().center()));
        event->accept();
        return;
    }
    
    // Update scene with mouse position for collaboration cursors
    QPointF scenePos = mapToScene(event->pos());
    m_scene->setLocalCursorPosition(scenePos);
    
    QGraphicsView::mouseMoveEvent(event);
}

void CanvasView::mouseReleaseEvent(QMouseEvent *event)
{
    if (m_isPanning && (event->button() == Qt::MiddleButton || 
        event->button() == Qt::LeftButton)) {
        m_isPanning = false;
        updateCursor();
        event->accept();
        return;
    }
    
    QGraphicsView::mouseReleaseEvent(event);
}

void CanvasView::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Space && !event->isAutoRepeat()) {
        m_isSpacePressed = true;
        setCursor(Qt::OpenHandCursor);
        event->accept();
        return;
    }
    
    QGraphicsView::keyPressEvent(event);
}

void CanvasView::keyReleaseEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Space && !event->isAutoRepeat()) {
        m_isSpacePressed = false;
        if (!m_isPanning) {
            updateCursor();
        }
        event->accept();
        return;
    }
    
    QGraphicsView::keyReleaseEvent(event);
}

void CanvasView::drawBackground(QPainter *painter, const QRectF &rect)
{
    painter->fillRect(rect, m_backgroundColor);
    
    if (m_showGrid && m_currentZoom > 0.2) {
        painter->setPen(QPen(m_gridColor, 0));
        
        qreal left = int(rect.left()) - (int(rect.left()) % m_gridSize);
        qreal top = int(rect.top()) - (int(rect.top()) % m_gridSize);
        
        QVarLengthArray<QLineF, 100> lines;
        
        for (qreal x = left; x < rect.right(); x += m_gridSize) {
            lines.append(QLineF(x, rect.top(), x, rect.bottom()));
        }
        
        for (qreal y = top; y < rect.bottom(); y += m_gridSize) {
            lines.append(QLineF(rect.left(), y, rect.right(), y));
        }
        
        painter->drawLines(lines.data(), lines.size());
    }
}

void CanvasView::resizeEvent(QResizeEvent *event)
{
    if (m_scaleWithWindow && m_lastViewportSize.isValid() && 
        m_lastViewportSize.width() > 0 && m_lastViewportSize.height() > 0) {
        // Calculate scale factor based on the average of width/height change
        QSize newSize = event->size();
        qreal widthRatio = static_cast<qreal>(newSize.width()) / m_lastViewportSize.width();
        qreal heightRatio = static_cast<qreal>(newSize.height()) / m_lastViewportSize.height();
        qreal scaleFactor = (widthRatio + heightRatio) / 2.0;
        
        // Apply the scale
        if (scaleFactor > 0 && qAbs(scaleFactor - 1.0) > 0.001) {
            QPointF center = mapToScene(viewport()->rect().center());
            applyZoom(scaleFactor, center);
        }
    }
    
    m_lastViewportSize = event->size();
    QGraphicsView::resizeEvent(event);
}

void CanvasView::setScaleWithWindow(bool enabled)
{
    if (m_scaleWithWindow != enabled) {
        m_scaleWithWindow = enabled;
        m_lastViewportSize = viewport()->size();
        emit scaleWithWindowChanged(enabled);
    }
}

void CanvasView::toggleScaleWithWindow()
{
    setScaleWithWindow(!m_scaleWithWindow);
}

void CanvasView::setGridVisible(bool visible)
{
    if (m_showGrid != visible) {
        m_showGrid = visible;
        viewport()->update();
    }
}

void CanvasView::updateCursor()
{
    if (m_isSpacePressed) {
        setCursor(Qt::OpenHandCursor);
    } else {
        setCursor(Qt::ArrowCursor);
    }
}

void CanvasView::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls() || event->mimeData()->hasImage()) {
        event->acceptProposedAction();
    }
}

void CanvasView::dragMoveEvent(QDragMoveEvent *event)
{
    if (event->mimeData()->hasUrls() || event->mimeData()->hasImage()) {
        event->acceptProposedAction();
    }
}

void CanvasView::dropEvent(QDropEvent *event)
{
    const QMimeData *mimeData = event->mimeData();
    QPointF scenePos = mapToScene(event->pos());
    
    if (mimeData->hasUrls()) {
        for (const QUrl &url : mimeData->urls()) {
            if (url.isLocalFile()) {
                QString filePath = url.toLocalFile();
                QImage image(filePath);
                if (!image.isNull()) {
                    m_scene->addImageItem(image, scenePos, filePath);
                    scenePos += QPointF(20, 20); // Offset for multiple images
                }
            }
        }
        event->acceptProposedAction();
    } else if (mimeData->hasImage()) {
        QImage image = qvariant_cast<QImage>(mimeData->imageData());
        if (!image.isNull()) {
            m_scene->addImageItem(image, scenePos);
        }
        event->acceptProposedAction();
    }
}
