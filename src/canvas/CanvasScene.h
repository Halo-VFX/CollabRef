#ifndef CANVASSCENE_H
#define CANVASSCENE_H

#include <QGraphicsScene>
#include <QList>
#include <QHash>
#include <QUndoStack>

class ImageItem;
class TextItem;
class Board;
class CursorWidget;
class SelectionRect;

struct RemoteCursor {
    QString oderId;
    QString userName;
    QColor color;
    QPointF position;
    QGraphicsItemGroup *widget;
};

class CanvasScene : public QGraphicsScene
{
    Q_OBJECT

public:
    explicit CanvasScene(QObject *parent = nullptr);
    ~CanvasScene();

    void setBoard(Board *board);
    Board *board() const { return m_board; }

    // Image operations
    ImageItem *addImageItem(const QImage &image, const QPointF &pos, 
                           const QString &sourcePath = QString());
    ImageItem *addImageItem(const QString &id, const QImage &image, 
                           const QPointF &pos, qreal rotation = 0,
                           qreal scale = 1.0);
    ImageItem *addImageItemFromFile(const QString &id, const QString &filePath, 
                           const QPointF &pos, qreal rotation = 0,
                           qreal scale = 1.0);
    void removeImageItem(const QString &id);
    void removeImageItem(ImageItem *item);
    ImageItem *findImageItem(const QString &id) const;
    QList<ImageItem*> imageItems() const;

    // Text operations
    TextItem *addTextItem(const QString &text, const QPointF &pos);
    TextItem *addTextItem(const QString &id, const QString &text, const QPointF &pos,
                          qreal rotation = 0);
    void removeTextItem(const QString &id);
    void removeTextItem(TextItem *item);
    TextItem *findTextItem(const QString &id) const;
    QList<TextItem*> textItems() const;

    // Selection
    QList<ImageItem*> selectedImageItems() const;
    QList<TextItem*> selectedTextItems() const;
    void selectAll();
    
    // Clipboard
    void pasteFromClipboard();
    void copySelected();
    
    // Edit operations
    void deleteSelected();
    void bringToFront();
    void sendToBack();
    void flipHorizontal();
    void flipVertical();
    void resetTransform();
    
    // Undo/Redo
    void undo();
    void redo();
    bool canUndo() const;
    bool canRedo() const;
    
    // Remote cursors
    void setLocalCursorPosition(const QPointF &pos);
    void updateRemoteCursor(const QString &oderId, const QString &userName,
                           const QPointF &pos, const QColor &color);
    void removeRemoteCursor(const QString &oderId);

signals:
    void imageAdded(ImageItem *item);
    void imageRemoved(const QString &id);
    void imageChanged(ImageItem *item);
    void textAdded(TextItem *item);
    void textRemoved(const QString &id);
    void textChanged(TextItem *item);
    void selectionChanged();
    void localCursorMoved(const QPointF &pos);
    void modificationChanged(bool modified);

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void dragEnterEvent(QGraphicsSceneDragDropEvent *event) override;
    void dragMoveEvent(QGraphicsSceneDragDropEvent *event) override;
    void dropEvent(QGraphicsSceneDragDropEvent *event) override;

private slots:
    void onItemChanged(ImageItem *item);
    void onTextItemChanged(TextItem *item);
    void onSelectionChanged();

private:
    void loadBoardItems();
    void clearAllItems();
    QString generateId() const;
    int nextZValue() const;

    Board *m_board;
    QHash<QString, ImageItem*> m_items;
    QHash<QString, TextItem*> m_textItems;
    QHash<QString, RemoteCursor> m_remoteCursors;
    QUndoStack *m_undoStack;
    
    // Marquee selection
    SelectionRect *m_selectionRect;
    QPointF m_selectionStart;
    bool m_isMarqueeSelecting;
    
    QPointF m_localCursorPos;
};

#endif // CANVASSCENE_H
