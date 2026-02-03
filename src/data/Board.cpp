#include "Board.h"

Board::Board(QObject *parent)
    : QObject(parent)
    , m_name("Untitled")
    , m_backgroundColor(35, 35, 38)
    , m_modified(false)
{
}

Board::~Board()
{
}

void Board::addImage(const BoardImage &image)
{
    m_images.insert(image.id, image);
    setModified(true);
    emit imageAdded(image.id);
    emit boardChanged();
}

void Board::removeImage(const QString &id)
{
    if (m_images.remove(id)) {
        setModified(true);
        emit imageRemoved(id);
        emit boardChanged();
    }
}

void Board::updateImage(const BoardImage &image)
{
    if (m_images.contains(image.id)) {
        m_images[image.id] = image;
        setModified(true);
        emit imageChanged(image.id);
        emit boardChanged();
    }
}

BoardImage Board::image(const QString &id) const
{
    return m_images.value(id);
}

QList<QString> Board::imageIds() const
{
    return m_images.keys();
}

int Board::imageCount() const
{
    return m_images.size();
}

void Board::setName(const QString &name)
{
    if (m_name != name) {
        m_name = name;
        setModified(true);
        emit boardChanged();
    }
}

void Board::setBackgroundColor(const QColor &color)
{
    if (m_backgroundColor != color) {
        m_backgroundColor = color;
        setModified(true);
        emit boardChanged();
    }
}

void Board::setModified(bool modified)
{
    if (m_modified != modified) {
        m_modified = modified;
        emit modifiedChanged(modified);
    }
}

void Board::clear()
{
    QList<QString> ids = m_images.keys();
    for (const QString &id : ids) {
        removeImage(id);
    }
    m_name = "Untitled";
    setModified(false);
}
