#include "CanvasScene.h"
#include "ImageItem.h"
#include "TextItem.h"
#include "SelectionRect.h"
#include "data/Board.h"

#include <QGraphicsSceneMouseEvent>
#include <QGraphicsSceneDragDropEvent>
#include <QKeyEvent>
#include <QMimeData>
#include <QClipboard>
#include <QApplication>
#include <QUrl>
#include <QUuid>
#include <QUndoCommand>
#include <QGraphicsItemGroup>
#include <QGraphicsEllipseItem>
#include <QGraphicsTextItem>
#include <QPen>
#include <QtMath>

// Undo commands
class AddImageCommand : public QUndoCommand
{
public:
    AddImageCommand(CanvasScene *scene, ImageItem *item)
        : m_scene(scene), m_item(item), m_id(item->id()) {
        setText("Add Image");
    }
    
    void undo() override {
        m_scene->removeImageItem(m_id);
    }
    
    void redo() override {
        if (!m_scene->findImageItem(m_id)) {
            // Re-add the item
            m_scene->addItem(m_item);
        }
    }
    
private:
    CanvasScene *m_scene;
    ImageItem *m_item;
    QString m_id;
};

class RemoveImageCommand : public QUndoCommand
{
public:
    RemoveImageCommand(CanvasScene *scene, ImageItem *item)
        : m_scene(scene), m_item(item) {
        setText("Remove Image");
    }
    
    void undo() override {
        m_scene->addItem(m_item);
    }
    
    void redo() override {
        m_scene->removeItem(m_item);
    }
    
private:
    CanvasScene *m_scene;
    ImageItem *m_item;
};

class MoveImageCommand : public QUndoCommand
{
public:
    MoveImageCommand(ImageItem *item, const QPointF &oldPos, const QPointF &newPos)
        : m_item(item), m_oldPos(oldPos), m_newPos(newPos) {
        setText("Move Image");
    }
    
    void undo() override {
        m_item->setPos(m_oldPos);
    }
    
    void redo() override {
        m_item->setPos(m_newPos);
    }
    
    int id() const override { return 1; }
    
    bool mergeWith(const QUndoCommand *other) override {
        if (other->id() != id()) return false;
        const MoveImageCommand *cmd = static_cast<const MoveImageCommand*>(other);
        if (cmd->m_item != m_item) return false;
        m_newPos = cmd->m_newPos;
        return true;
    }
    
private:
    ImageItem *m_item;
    QPointF m_oldPos;
    QPointF m_newPos;
};

CanvasScene::CanvasScene(QObject *parent)
    : QGraphicsScene(parent)
    , m_board(nullptr)
    , m_undoStack(new QUndoStack(this))
    , m_selectionRect(nullptr)
    , m_isMarqueeSelecting(false)
{
    setSceneRect(-50000, -50000, 100000, 100000);
    
    connect(this, &QGraphicsScene::selectionChanged,
            this, &CanvasScene::onSelectionChanged);
}

CanvasScene::~CanvasScene()
{
}

void CanvasScene::setBoard(Board *board)
{
    if (m_board) {
        disconnect(m_board, nullptr, this, nullptr);
    }
    
    clearAllItems();
    m_board = board;
    
    if (m_board) {
        loadBoardItems();
        connect(m_board, &Board::imageAdded, this, [this](const QString &id) {
            // Handle remote additions
            if (!m_items.contains(id)) {
                BoardImage img = m_board->image(id);
                addImageItem(id, img.image, img.position, img.rotation, img.scale);
            }
        });
        connect(m_board, &Board::imageRemoved, this, [this](const QString &id) {
            if (m_items.contains(id)) {
                removeImageItem(id);
            }
        });
        connect(m_board, &Board::imageChanged, this, [this](const QString &id) {
            // Handle remote changes
            if (ImageItem *item = m_items.value(id)) {
                BoardImage img = m_board->image(id);
                item->setPos(img.position);
                item->setRotation(img.rotation);
                item->setScale(img.scale);
                item->setZValue(img.zIndex);
            }
        });
    }
    
    m_undoStack->clear();
}

void CanvasScene::loadBoardItems()
{
    if (!m_board) return;
    
    for (const QString &id : m_board->imageIds()) {
        BoardImage img = m_board->image(id);
        addImageItem(id, img.image, img.position, img.rotation, img.scale);
    }
}

void CanvasScene::clearAllItems()
{
    for (ImageItem *item : m_items) {
        removeItem(item);
        delete item;
    }
    m_items.clear();
}

ImageItem *CanvasScene::addImageItem(const QImage &image, const QPointF &pos,
                                     const QString &sourcePath)
{
    QString id = generateId();
    ImageItem *item;
    
    // Check if this is a GIF - use filepath constructor for animation support
    if (!sourcePath.isEmpty() && sourcePath.toLower().endsWith(".gif")) {
        item = new ImageItem(id, sourcePath);
    } else {
        item = new ImageItem(id, image);
        item->setSourcePath(sourcePath);
    }
    
    item->setPos(pos);
    item->setZValue(nextZValue());
    
    m_items.insert(id, item);
    addItem(item);
    
    connect(item, &ImageItem::itemChanged, this, &CanvasScene::onItemChanged);
    
    // Add to board
    if (m_board) {
        BoardImage boardImg;
        boardImg.id = id;
        boardImg.image = item->image(); // Get current frame for GIFs
        boardImg.position = pos;
        boardImg.rotation = 0;
        boardImg.scale = 1.0;
        boardImg.zIndex = item->zValue();
        boardImg.sourcePath = sourcePath;
        m_board->addImage(boardImg);
    }
    
    emit imageAdded(item);
    emit modificationChanged(true);
    
    return item;
}

ImageItem *CanvasScene::addImageItem(const QString &id, const QImage &image,
                                     const QPointF &pos, qreal rotation, qreal scale)
{
    if (m_items.contains(id)) {
        return m_items.value(id);
    }
    
    ImageItem *item = new ImageItem(id, image);
    item->setPos(pos);
    item->setRotation(rotation);
    item->setScale(scale);
    item->setZValue(nextZValue());
    
    m_items.insert(id, item);
    addItem(item);
    
    connect(item, &ImageItem::itemChanged, this, &CanvasScene::onItemChanged);
    
    emit imageAdded(item);
    
    return item;
}

ImageItem *CanvasScene::addImageItemFromFile(const QString &id, const QString &filePath,
                                     const QPointF &pos, qreal rotation, qreal scale)
{
    if (m_items.contains(id)) {
        return m_items.value(id);
    }
    
    // Use file path constructor for GIF animation support
    ImageItem *item = new ImageItem(id, filePath);
    item->setPos(pos);
    item->setRotation(rotation);
    item->setScale(scale);
    item->setZValue(nextZValue());
    
    m_items.insert(id, item);
    addItem(item);
    
    connect(item, &ImageItem::itemChanged, this, &CanvasScene::onItemChanged);
    
    emit imageAdded(item);
    
    return item;
}

void CanvasScene::removeImageItem(const QString &id)
{
    if (ImageItem *item = m_items.take(id)) {
        removeItem(item);
        emit imageRemoved(id);
        emit modificationChanged(true);
        
        if (m_board) {
            m_board->removeImage(id);
        }
        
        delete item;
    }
}

void CanvasScene::removeImageItem(ImageItem *item)
{
    if (item) {
        removeImageItem(item->id());
    }
}

ImageItem *CanvasScene::findImageItem(const QString &id) const
{
    return m_items.value(id, nullptr);
}

QList<ImageItem*> CanvasScene::imageItems() const
{
    return m_items.values();
}

// Text operations
TextItem *CanvasScene::addTextItem(const QString &text, const QPointF &pos)
{
    QString id = generateId();
    return addTextItem(id, text, pos);
}

TextItem *CanvasScene::addTextItem(const QString &id, const QString &text, 
                                    const QPointF &pos, qreal rotation)
{
    TextItem *item = new TextItem(id, text);
    item->setPos(pos);
    item->setRotation(rotation);
    item->setZValue(nextZValue());
    
    addItem(item);
    m_textItems.insert(id, item);
    
    connect(item, &TextItem::textChanged, this, &CanvasScene::onTextItemChanged);
    
    emit textAdded(item);
    emit modificationChanged(true);
    
    return item;
}

void CanvasScene::removeTextItem(const QString &id)
{
    if (TextItem *item = m_textItems.value(id)) {
        removeTextItem(item);
    }
}

void CanvasScene::removeTextItem(TextItem *item)
{
    if (!item) return;
    
    QString id = item->id();
    m_textItems.remove(id);
    removeItem(item);
    delete item;
    
    emit textRemoved(id);
    emit modificationChanged(true);
}

TextItem *CanvasScene::findTextItem(const QString &id) const
{
    return m_textItems.value(id, nullptr);
}

QList<TextItem*> CanvasScene::textItems() const
{
    return m_textItems.values();
}

QList<ImageItem*> CanvasScene::selectedImageItems() const
{
    QList<ImageItem*> result;
    for (QGraphicsItem *item : selectedItems()) {
        if (ImageItem *imageItem = dynamic_cast<ImageItem*>(item)) {
            result.append(imageItem);
        }
    }
    return result;
}

QList<TextItem*> CanvasScene::selectedTextItems() const
{
    QList<TextItem*> result;
    for (QGraphicsItem *item : selectedItems()) {
        if (TextItem *textItem = dynamic_cast<TextItem*>(item)) {
            result.append(textItem);
        }
    }
    return result;
}

void CanvasScene::selectAll()
{
    for (ImageItem *item : m_items) {
        item->setSelected(true);
    }
    for (TextItem *item : m_textItems) {
        item->setSelected(true);
    }
}

void CanvasScene::pasteFromClipboard()
{
    const QClipboard *clipboard = QApplication::clipboard();
    const QMimeData *mimeData = clipboard->mimeData();
    
    QPointF pastePos = m_localCursorPos;
    if (pastePos.isNull()) {
        pastePos = QPointF(0, 0);
    }
    
    // Try to get image from clipboard - check multiple formats
    QImage image;
    
    // First try direct image data
    if (mimeData->hasImage()) {
        image = qvariant_cast<QImage>(mimeData->imageData());
    }
    
    // If no image, try URLs (for files copied from Explorer)
    if (image.isNull() && mimeData->hasUrls()) {
        for (const QUrl &url : mimeData->urls()) {
            if (url.isLocalFile()) {
                QString filePath = url.toLocalFile();
                image = QImage(filePath);
                if (!image.isNull()) {
                    addImageItem(image, pastePos, filePath);
                    pastePos += QPointF(20, 20);
                }
            }
        }
        return; // Already added via URLs
    }
    
    // Try Windows-specific formats if still no image
    if (image.isNull()) {
        // Try to load from "application/x-qt-image"
        QByteArray data = mimeData->data("application/x-qt-image");
        if (!data.isEmpty()) {
            image.loadFromData(data);
        }
    }
    
    if (!image.isNull()) {
        addImageItem(image, pastePos);
    }
}

void CanvasScene::copySelected()
{
    QList<ImageItem*> selected = selectedImageItems();
    if (selected.isEmpty()) return;
    
    // For now, just copy the first selected image
    ImageItem *item = selected.first();
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setImage(item->image());
}

void CanvasScene::deleteSelected()
{
    QList<ImageItem*> selectedImages = selectedImageItems();
    for (ImageItem *item : selectedImages) {
        removeImageItem(item);
    }
    
    QList<TextItem*> selectedTexts = selectedTextItems();
    for (TextItem *item : selectedTexts) {
        removeTextItem(item);
    }
}

void CanvasScene::bringToFront()
{
    qreal maxZ = 0;
    for (ImageItem *item : m_items) {
        maxZ = qMax(maxZ, item->zValue());
    }
    
    for (ImageItem *item : selectedImageItems()) {
        item->setZValue(++maxZ);
    }
}

void CanvasScene::sendToBack()
{
    qreal minZ = 0;
    for (ImageItem *item : m_items) {
        minZ = qMin(minZ, item->zValue());
    }
    
    for (ImageItem *item : selectedImageItems()) {
        item->setZValue(--minZ);
    }
}

void CanvasScene::flipHorizontal()
{
    for (ImageItem *item : selectedImageItems()) {
        item->flipHorizontal();
    }
}

void CanvasScene::flipVertical()
{
    for (ImageItem *item : selectedImageItems()) {
        item->flipVertical();
    }
}

void CanvasScene::resetTransform()
{
    for (ImageItem *item : selectedImageItems()) {
        item->resetTransform();
    }
}

void CanvasScene::undo()
{
    if (m_undoStack->canUndo()) {
        m_undoStack->undo();
    }
}

void CanvasScene::redo()
{
    if (m_undoStack->canRedo()) {
        m_undoStack->redo();
    }
}

bool CanvasScene::canUndo() const
{
    return m_undoStack->canUndo();
}

bool CanvasScene::canRedo() const
{
    return m_undoStack->canRedo();
}

void CanvasScene::setLocalCursorPosition(const QPointF &pos)
{
    m_localCursorPos = pos;
    emit localCursorMoved(pos);
}

void CanvasScene::updateRemoteCursor(const QString &oderId, const QString &userName,
                                     const QPointF &pos, const QColor &color)
{
    if (!m_remoteCursors.contains(oderId)) {
        // Create cursor widget
        RemoteCursor cursor;
        cursor.oderId = oderId;
        cursor.userName = userName;
        cursor.color = color;
        cursor.position = pos;
        
        // Create visual representation
        QGraphicsItemGroup *group = new QGraphicsItemGroup();
        
        // Cursor dot
        QGraphicsEllipseItem *dot = new QGraphicsEllipseItem(-5, -5, 10, 10);
        dot->setBrush(color);
        dot->setPen(QPen(color.darker(120), 2));
        group->addToGroup(dot);
        
        // Name label
        QGraphicsTextItem *label = new QGraphicsTextItem(userName);
        label->setDefaultTextColor(color);
        label->setPos(10, -5);
        label->setFont(QFont("Arial", 10));
        group->addToGroup(label);
        
        group->setPos(pos);
        group->setZValue(10000); // Always on top
        addItem(group);
        
        cursor.widget = group;
        m_remoteCursors.insert(oderId, cursor);
    } else {
        // Update existing cursor
        RemoteCursor &cursor = m_remoteCursors[oderId];
        cursor.position = pos;
        if (cursor.widget) {
            cursor.widget->setPos(pos);
        }
    }
}

void CanvasScene::removeRemoteCursor(const QString &oderId)
{
    if (m_remoteCursors.contains(oderId)) {
        RemoteCursor cursor = m_remoteCursors.take(oderId);
        if (cursor.widget) {
            removeItem(cursor.widget);
            delete cursor.widget;
        }
    }
}

void CanvasScene::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        QGraphicsItem *item = itemAt(event->scenePos(), QTransform());
        
        // Start marquee selection if clicking on empty space
        if (!item || item->type() == QGraphicsItemGroup::Type) {
            if (!m_selectionRect) {
                m_selectionRect = new SelectionRect();
                addItem(m_selectionRect);
            }
            m_selectionStart = event->scenePos();
            m_selectionRect->setRect(QRectF(m_selectionStart, m_selectionStart));
            m_selectionRect->show();
            m_isMarqueeSelecting = true;
            
            if (!(event->modifiers() & Qt::ShiftModifier)) {
                clearSelection();
            }
        }
    }
    
    QGraphicsScene::mousePressEvent(event);
}

void CanvasScene::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    // Always emit cursor position for collaboration
    emit localCursorMoved(event->scenePos());
    
    if (m_isMarqueeSelecting && m_selectionRect) {
        QRectF rect = QRectF(m_selectionStart, event->scenePos()).normalized();
        m_selectionRect->setRect(rect);
        
        // Update selection
        QPainterPath path;
        path.addRect(rect);
        setSelectionArea(path, Qt::ReplaceSelection, Qt::IntersectsItemShape);
    }
    
    QGraphicsScene::mouseMoveEvent(event);
}

void CanvasScene::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    if (m_isMarqueeSelecting) {
        m_isMarqueeSelecting = false;
        if (m_selectionRect) {
            m_selectionRect->hide();
        }
    }
    
    QGraphicsScene::mouseReleaseEvent(event);
}

void CanvasScene::keyPressEvent(QKeyEvent *event)
{
    // Arrow key nudging
    if (!selectedItems().isEmpty()) {
        qreal nudge = (event->modifiers() & Qt::ShiftModifier) ? 10 : 1;
        QPointF delta;
        
        switch (event->key()) {
            case Qt::Key_Left:  delta = QPointF(-nudge, 0); break;
            case Qt::Key_Right: delta = QPointF(nudge, 0); break;
            case Qt::Key_Up:    delta = QPointF(0, -nudge); break;
            case Qt::Key_Down:  delta = QPointF(0, nudge); break;
            default: break;
        }
        
        if (!delta.isNull()) {
            for (ImageItem *item : selectedImageItems()) {
                item->setPos(item->pos() + delta);
            }
            event->accept();
            return;
        }
    }
    
    QGraphicsScene::keyPressEvent(event);
}

void CanvasScene::dragEnterEvent(QGraphicsSceneDragDropEvent *event)
{
    if (event->mimeData()->hasUrls() || event->mimeData()->hasImage()) {
        event->acceptProposedAction();
    }
}

void CanvasScene::dragMoveEvent(QGraphicsSceneDragDropEvent *event)
{
    if (event->mimeData()->hasUrls() || event->mimeData()->hasImage()) {
        event->acceptProposedAction();
    }
}

void CanvasScene::dropEvent(QGraphicsSceneDragDropEvent *event)
{
    const QMimeData *mimeData = event->mimeData();
    QPointF dropPos = event->scenePos();
    
    if (mimeData->hasUrls()) {
        for (const QUrl &url : mimeData->urls()) {
            if (url.isLocalFile()) {
                QString filePath = url.toLocalFile();
                QImage image(filePath);
                if (!image.isNull()) {
                    addImageItem(image, dropPos, filePath);
                    dropPos += QPointF(20, 20);
                }
            }
        }
        event->acceptProposedAction();
    } else if (mimeData->hasImage()) {
        QImage image = qvariant_cast<QImage>(mimeData->imageData());
        if (!image.isNull()) {
            addImageItem(image, dropPos);
        }
        event->acceptProposedAction();
    }
}

void CanvasScene::onItemChanged(ImageItem *item)
{
    emit imageChanged(item);
    emit modificationChanged(true);
    
    // Update board
    if (m_board && item) {
        BoardImage img = m_board->image(item->id());
        img.position = item->pos();
        img.rotation = item->rotation();
        img.scale = item->scale();
        img.zIndex = item->zValue();
        m_board->updateImage(img);
    }
}

void CanvasScene::onTextItemChanged(TextItem *item)
{
    emit textChanged(item);
    emit modificationChanged(true);
}

void CanvasScene::onSelectionChanged()
{
    emit selectionChanged();
}

QString CanvasScene::generateId() const
{
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

int CanvasScene::nextZValue() const
{
    int maxZ = 0;
    for (ImageItem *item : m_items) {
        maxZ = qMax(maxZ, static_cast<int>(item->zValue()));
    }
    return maxZ + 1;
}
