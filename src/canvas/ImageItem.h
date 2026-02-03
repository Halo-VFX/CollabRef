#ifndef IMAGEITEM_H
#define IMAGEITEM_H

#include <QGraphicsObject>
#include <QImage>
#include <QPixmap>
#include <QMovie>
#include <QPointer>

class ImageItem : public QGraphicsObject
{
    Q_OBJECT

public:
    enum { Type = UserType + 1 };
    
    explicit ImageItem(const QString &id, const QImage &image, 
                      QGraphicsItem *parent = nullptr);
    explicit ImageItem(const QString &id, const QString &filePath,
                      QGraphicsItem *parent = nullptr);
    ~ImageItem();

    int type() const override { return Type; }
    
    QString id() const { return m_id; }
    QImage image() const { return m_image; }
    QString sourcePath() const { return m_sourcePath; }
    void setSourcePath(const QString &path);
    
    bool isAnimated() const { return m_movie != nullptr; }
    
    // Transform operations
    void flipHorizontal();
    void flipVertical();
    void resetTransform();
    void setImageRotation(qreal angle);
    void setImageScale(qreal scale);
    
    // Crop operations
    void setCrop(const QRectF &cropRect);
    QRectF crop() const { return m_cropRect; }
    void resetCrop();
    
    // Properties
    bool isFlippedHorizontally() const { return m_flippedH; }
    bool isFlippedVertically() const { return m_flippedV; }
    
    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
               QWidget *widget) override;
    QPainterPath shape() const override;

signals:
    void itemChanged(ImageItem *item);
    void itemMoved(ImageItem *item);
    void itemScaled(ImageItem *item);
    void itemRotated(ImageItem *item);

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
    void hoverEnterEvent(QGraphicsSceneHoverEvent *event) override;
    void hoverMoveEvent(QGraphicsSceneHoverEvent *event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) override;
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;

private slots:
    void onMovieFrameChanged();

private:
    enum Handle {
        NoHandle = 0,
        TopLeft, Top, TopRight,
        Left, Right,
        BottomLeft, Bottom, BottomRight,
        Rotate
    };
    
    Handle handleAt(const QPointF &pos) const;
    QRectF handleRect(Handle handle) const;
    void updateCursor(Handle handle);
    void drawHandles(QPainter *painter);
    void updatePixmap();
    void setupAnimation(const QString &filePath);
    
    QString m_id;
    QImage m_image;
    QPixmap m_pixmap;
    QString m_sourcePath;
    
    // Animation support
    QPointer<QMovie> m_movie;
    
    QRectF m_cropRect;
    bool m_flippedH;
    bool m_flippedV;
    
    // Interaction state
    Handle m_currentHandle;
    QPointF m_dragStart;
    QPointF m_originalPos;
    QRectF m_originalRect;
    qreal m_originalRotation;
    qreal m_originalScale;
    bool m_isMoving;
    bool m_isResizing;
    bool m_isRotating;
    
    static constexpr qreal HANDLE_SIZE = 10.0;
    static constexpr qreal ROTATE_HANDLE_DISTANCE = 30.0;
};

#endif // IMAGEITEM_H
