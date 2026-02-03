#include "ImageItem.h"

#include <QPainter>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsSceneHoverEvent>
#include <QStyleOptionGraphicsItem>
#include <QCursor>
#include <QtMath>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QFileInfo>
#include <QImageReader>

ImageItem::ImageItem(const QString &id, const QImage &image, QGraphicsItem *parent)
    : QGraphicsObject(parent)
    , m_id(id)
    , m_image(image)
    , m_movie(nullptr)
    , m_flippedH(false)
    , m_flippedV(false)
    , m_currentHandle(NoHandle)
    , m_isMoving(false)
    , m_isResizing(false)
    , m_isRotating(false)
{
    setFlags(QGraphicsItem::ItemIsMovable |
             QGraphicsItem::ItemIsSelectable |
             QGraphicsItem::ItemSendsGeometryChanges);
    setAcceptHoverEvents(true);
    
    m_cropRect = QRectF(QPointF(0, 0), QSizeF(m_image.size()));
    updatePixmap();
    
    setVisible(true);
    setEnabled(true);
}

ImageItem::ImageItem(const QString &id, const QString &filePath, QGraphicsItem *parent)
    : QGraphicsObject(parent)
    , m_id(id)
    , m_sourcePath(filePath)
    , m_movie(nullptr)
    , m_flippedH(false)
    , m_flippedV(false)
    , m_currentHandle(NoHandle)
    , m_isMoving(false)
    , m_isResizing(false)
    , m_isRotating(false)
{
    setFlags(QGraphicsItem::ItemIsMovable |
             QGraphicsItem::ItemIsSelectable |
             QGraphicsItem::ItemSendsGeometryChanges);
    setAcceptHoverEvents(true);
    
    // Check if this is an animated format
    QFileInfo fileInfo(filePath);
    QString suffix = fileInfo.suffix().toLower();
    
    if (suffix == "gif") {
        setupAnimation(filePath);
    } else {
        m_image = QImage(filePath);
        m_cropRect = QRectF(QPointF(0, 0), QSizeF(m_image.size()));
        updatePixmap();
    }
    
    setVisible(true);
    setEnabled(true);
}

void ImageItem::setupAnimation(const QString &filePath)
{
    m_movie = new QMovie(filePath, QByteArray(), this);
    
    if (m_movie->isValid()) {
        connect(m_movie, &QMovie::frameChanged, this, &ImageItem::onMovieFrameChanged);
        m_movie->start();
        
        // Get first frame for size
        m_image = m_movie->currentImage();
        m_cropRect = QRectF(QPointF(0, 0), QSizeF(m_image.size()));
        updatePixmap();
    } else {
        // Fallback to static image
        delete m_movie;
        m_movie = nullptr;
        m_image = QImage(filePath);
        m_cropRect = QRectF(QPointF(0, 0), QSizeF(m_image.size()));
        updatePixmap();
    }
}

void ImageItem::onMovieFrameChanged()
{
    if (m_movie) {
        m_image = m_movie->currentImage();
        updatePixmap();
        update();
    }
}

void ImageItem::setSourcePath(const QString &path)
{
    m_sourcePath = path;
    
    // Check if this is an animated GIF and set up animation
    QFileInfo fileInfo(path);
    if (fileInfo.suffix().toLower() == "gif" && !m_movie) {
        setupAnimation(path);
    }
}

ImageItem::~ImageItem()
{
    if (m_movie) {
        m_movie->stop();
        delete m_movie;
    }
}

void ImageItem::flipHorizontal()
{
    m_flippedH = !m_flippedH;
    updatePixmap();
    update();
    emit itemChanged(this);
}

void ImageItem::flipVertical()
{
    m_flippedV = !m_flippedV;
    updatePixmap();
    update();
    emit itemChanged(this);
}

void ImageItem::resetTransform()
{
    setRotation(0);
    setScale(1.0);
    m_flippedH = false;
    m_flippedV = false;
    updatePixmap();
    update();
    emit itemChanged(this);
}

void ImageItem::setImageRotation(qreal angle)
{
    setRotation(angle);
    emit itemRotated(this);
    emit itemChanged(this);
}

void ImageItem::setImageScale(qreal s)
{
    setScale(s);
    emit itemScaled(this);
    emit itemChanged(this);
}

void ImageItem::setCrop(const QRectF &cropRect)
{
    m_cropRect = cropRect.intersected(QRectF(QPointF(0, 0), m_image.size()));
    updatePixmap();
    update();
    emit itemChanged(this);
}

void ImageItem::resetCrop()
{
    m_cropRect = QRectF(QPointF(0, 0), m_image.size());
    updatePixmap();
    update();
    emit itemChanged(this);
}

QRectF ImageItem::boundingRect() const
{
    qreal extra = HANDLE_SIZE + ROTATE_HANDLE_DISTANCE + 5;
    return QRectF(-m_cropRect.width() / 2 - extra,
                  -m_cropRect.height() / 2 - extra - ROTATE_HANDLE_DISTANCE,
                  m_cropRect.width() + extra * 2,
                  m_cropRect.height() + extra * 2 + ROTATE_HANDLE_DISTANCE);
}

void ImageItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
                      QWidget *widget)
{
    Q_UNUSED(widget)
    
    painter->setRenderHint(QPainter::SmoothPixmapTransform);
    
    // Draw image centered at origin
    QRectF destRect(-m_cropRect.width() / 2, -m_cropRect.height() / 2,
                    m_cropRect.width(), m_cropRect.height());
    
    // Debug: draw a colored rectangle first so we know the item is painting
    painter->fillRect(destRect, QColor(100, 100, 100, 128));
    
    if (!m_pixmap.isNull()) {
        painter->drawPixmap(destRect.toRect(), m_pixmap);
    } else {
        // Draw red X if pixmap is null
        painter->setPen(QPen(Qt::red, 3));
        painter->drawLine(destRect.topLeft(), destRect.bottomRight());
        painter->drawLine(destRect.topRight(), destRect.bottomLeft());
    }
    
    // Always draw border so we can see where items are
    painter->setPen(QPen(QColor(42, 130, 218), 2, Qt::SolidLine));
    painter->setBrush(Qt::NoBrush);
    painter->drawRect(destRect);
    
    // Draw selection handles
    if (option->state & QStyle::State_Selected) {
        drawHandles(painter);
    }
}

QPainterPath ImageItem::shape() const
{
    QPainterPath path;
    path.addRect(QRectF(-m_cropRect.width() / 2, -m_cropRect.height() / 2,
                        m_cropRect.width(), m_cropRect.height()));
    return path;
}

void ImageItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && isSelected()) {
        m_currentHandle = handleAt(event->pos());
        m_dragStart = event->scenePos();
        m_originalPos = pos();
        m_originalRect = QRectF(-m_cropRect.width() / 2, -m_cropRect.height() / 2,
                                m_cropRect.width(), m_cropRect.height());
        m_originalRotation = rotation();
        m_originalScale = scale();
        
        if (m_currentHandle == Rotate) {
            m_isRotating = true;
            event->accept();
            return;
        } else if (m_currentHandle != NoHandle) {
            m_isResizing = true;
            event->accept();
            return;
        }
    }
    
    m_isMoving = true;
    QGraphicsObject::mousePressEvent(event);
}

void ImageItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if (m_isRotating) {
        // Calculate rotation based on angle from center
        QPointF center = mapToScene(QPointF(0, 0));
        QPointF currentPos = event->scenePos();
        
        qreal angle = qAtan2(currentPos.y() - center.y(),
                            currentPos.x() - center.x());
        qreal startAngle = qAtan2(m_dragStart.y() - center.y(),
                                  m_dragStart.x() - center.x());
        
        qreal deltaAngle = qRadiansToDegrees(angle - startAngle);
        qreal newRotation = m_originalRotation + deltaAngle;
        
        // Snap to 45 degree increments if shift is held
        if (event->modifiers() & Qt::ShiftModifier) {
            newRotation = qRound(newRotation / 45.0) * 45.0;
        }
        
        setRotation(newRotation);
        emit itemRotated(this);
        event->accept();
        return;
    }
    
    if (m_isResizing) {
        QPointF delta = event->scenePos() - m_dragStart;
        
        // Transform delta to local coordinates
        qreal angle = qDegreesToRadians(rotation());
        QPointF localDelta(delta.x() * qCos(-angle) - delta.y() * qSin(-angle),
                          delta.x() * qSin(-angle) + delta.y() * qCos(-angle));
        
        // Calculate new scale based on handle
        qreal scaleX = 1.0, scaleY = 1.0;
        
        switch (m_currentHandle) {
            case TopLeft:
            case BottomLeft:
            case Left:
                scaleX = (m_originalRect.width() - localDelta.x()) / m_originalRect.width();
                break;
            case TopRight:
            case BottomRight:
            case Right:
                scaleX = (m_originalRect.width() + localDelta.x()) / m_originalRect.width();
                break;
            default:
                break;
        }
        
        switch (m_currentHandle) {
            case TopLeft:
            case TopRight:
            case Top:
                scaleY = (m_originalRect.height() - localDelta.y()) / m_originalRect.height();
                break;
            case BottomLeft:
            case BottomRight:
            case Bottom:
                scaleY = (m_originalRect.height() + localDelta.y()) / m_originalRect.height();
                break;
            default:
                break;
        }
        
        // Maintain aspect ratio if Shift is held
        if (event->modifiers() & Qt::ShiftModifier) {
            qreal avgScale = (qAbs(scaleX) + qAbs(scaleY)) / 2;
            scaleX = scaleX > 0 ? avgScale : -avgScale;
            scaleY = scaleY > 0 ? avgScale : -avgScale;
        }
        
        // Apply scale
        qreal newScale = m_originalScale * qMax(qAbs(scaleX), qAbs(scaleY));
        newScale = qBound(0.01, newScale, 100.0);
        
        setScale(newScale);
        emit itemScaled(this);
        event->accept();
        return;
    }
    
    QGraphicsObject::mouseMoveEvent(event);
}

void ImageItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    if (m_isRotating || m_isResizing || m_isMoving) {
        emit itemChanged(this);
    }
    
    m_isRotating = false;
    m_isResizing = false;
    m_isMoving = false;
    m_currentHandle = NoHandle;
    
    QGraphicsObject::mouseReleaseEvent(event);
}

void ImageItem::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
    if (isSelected()) {
        updateCursor(handleAt(event->pos()));
    }
    QGraphicsObject::hoverEnterEvent(event);
}

void ImageItem::hoverMoveEvent(QGraphicsSceneHoverEvent *event)
{
    if (isSelected()) {
        updateCursor(handleAt(event->pos()));
    }
    QGraphicsObject::hoverMoveEvent(event);
}

void ImageItem::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
    setCursor(Qt::ArrowCursor);
    QGraphicsObject::hoverLeaveEvent(event);
}

QVariant ImageItem::itemChange(GraphicsItemChange change, const QVariant &value)
{
    if (change == ItemPositionHasChanged) {
        emit itemMoved(this);
        emit itemChanged(this);
    } else if (change == ItemSelectedHasChanged) {
        update();
    }
    
    return QGraphicsObject::itemChange(change, value);
}

ImageItem::Handle ImageItem::handleAt(const QPointF &pos) const
{
    // Check rotate handle first
    QRectF rotateRect = handleRect(Rotate);
    if (rotateRect.contains(pos)) {
        return Rotate;
    }
    
    // Check resize handles
    static const Handle handles[] = {
        TopLeft, Top, TopRight,
        Left, Right,
        BottomLeft, Bottom, BottomRight
    };
    
    for (Handle h : handles) {
        if (handleRect(h).contains(pos)) {
            return h;
        }
    }
    
    return NoHandle;
}

QRectF ImageItem::handleRect(Handle handle) const
{
    qreal w = m_cropRect.width() / 2;
    qreal h = m_cropRect.height() / 2;
    qreal hs = HANDLE_SIZE / 2;
    
    switch (handle) {
        case TopLeft:     return QRectF(-w - hs, -h - hs, HANDLE_SIZE, HANDLE_SIZE);
        case Top:         return QRectF(-hs, -h - hs, HANDLE_SIZE, HANDLE_SIZE);
        case TopRight:    return QRectF(w - hs, -h - hs, HANDLE_SIZE, HANDLE_SIZE);
        case Left:        return QRectF(-w - hs, -hs, HANDLE_SIZE, HANDLE_SIZE);
        case Right:       return QRectF(w - hs, -hs, HANDLE_SIZE, HANDLE_SIZE);
        case BottomLeft:  return QRectF(-w - hs, h - hs, HANDLE_SIZE, HANDLE_SIZE);
        case Bottom:      return QRectF(-hs, h - hs, HANDLE_SIZE, HANDLE_SIZE);
        case BottomRight: return QRectF(w - hs, h - hs, HANDLE_SIZE, HANDLE_SIZE);
        case Rotate:      return QRectF(-hs, -h - ROTATE_HANDLE_DISTANCE - hs, 
                                       HANDLE_SIZE, HANDLE_SIZE);
        default:          return QRectF();
    }
}

void ImageItem::updateCursor(Handle handle)
{
    switch (handle) {
        case TopLeft:
        case BottomRight:
            setCursor(Qt::SizeFDiagCursor);
            break;
        case TopRight:
        case BottomLeft:
            setCursor(Qt::SizeBDiagCursor);
            break;
        case Top:
        case Bottom:
            setCursor(Qt::SizeVerCursor);
            break;
        case Left:
        case Right:
            setCursor(Qt::SizeHorCursor);
            break;
        case Rotate:
            // Custom rotate cursor would be nice
            setCursor(Qt::CrossCursor);
            break;
        default:
            setCursor(Qt::ArrowCursor);
            break;
    }
}

void ImageItem::drawHandles(QPainter *painter)
{
    painter->setBrush(Qt::white);
    painter->setPen(QPen(QColor(42, 130, 218), 1));
    
    // Corner and edge handles
    static const Handle handles[] = {
        TopLeft, Top, TopRight,
        Left, Right,
        BottomLeft, Bottom, BottomRight
    };
    
    for (Handle h : handles) {
        painter->drawRect(handleRect(h));
    }
    
    // Rotate handle
    qreal h = m_cropRect.height() / 2;
    painter->setPen(QPen(QColor(42, 130, 218), 1, Qt::DashLine));
    painter->drawLine(QPointF(0, -h), QPointF(0, -h - ROTATE_HANDLE_DISTANCE));
    
    painter->setPen(QPen(QColor(42, 130, 218), 1));
    painter->setBrush(QColor(42, 130, 218));
    painter->drawEllipse(handleRect(Rotate));
}

void ImageItem::updatePixmap()
{
    QImage cropped = m_image.copy(m_cropRect.toRect());
    
    // Apply flips
    if (m_flippedH || m_flippedV) {
        cropped = cropped.mirrored(m_flippedH, m_flippedV);
    }
    
    m_pixmap = QPixmap::fromImage(cropped);
}
