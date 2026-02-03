#ifndef BOARD_H
#define BOARD_H

#include <QObject>
#include <QHash>
#include <QImage>
#include <QPointF>

struct BoardImage {
    QString id;
    QImage image;
    QPointF position;
    qreal rotation = 0;
    qreal scale = 1.0;
    qreal zIndex = 0;
    QString sourcePath;
    QRectF cropRect;
    bool flippedH = false;
    bool flippedV = false;
};

class Board : public QObject
{
    Q_OBJECT

public:
    explicit Board(QObject *parent = nullptr);
    ~Board();

    // Image operations
    void addImage(const BoardImage &image);
    void removeImage(const QString &id);
    void updateImage(const BoardImage &image);
    BoardImage image(const QString &id) const;
    QList<QString> imageIds() const;
    int imageCount() const;
    
    // Board metadata
    QString name() const { return m_name; }
    void setName(const QString &name);
    
    QColor backgroundColor() const { return m_backgroundColor; }
    void setBackgroundColor(const QColor &color);
    
    // State
    bool isModified() const { return m_modified; }
    void setModified(bool modified);
    void clear();

signals:
    void imageAdded(const QString &id);
    void imageRemoved(const QString &id);
    void imageChanged(const QString &id);
    void boardChanged();
    void modifiedChanged(bool modified);

private:
    QHash<QString, BoardImage> m_images;
    QString m_name;
    QColor m_backgroundColor;
    bool m_modified;
};

#endif // BOARD_H
