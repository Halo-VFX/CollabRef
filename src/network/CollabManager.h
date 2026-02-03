#ifndef COLLABMANAGER_H
#define COLLABMANAGER_H

#include <QObject>
#include <QHash>
#include <QColor>
#include <QTimer>
#include <QPointF>

class SyncClient;
class Board;
class CanvasScene;
class ImageItem;
class TextItem;

struct Collaborator {
    QString oderId;
    QString userName;
    QColor color;
    QPointF cursorPos;
    bool isActive;
};

class CollabManager : public QObject
{
    Q_OBJECT

public:
    explicit CollabManager(QObject *parent = nullptr);
    ~CollabManager();

    void setBoard(Board *board);
    void setScene(CanvasScene *scene);
    
    void connectToServer(const QString &url, const QString &roomId);
    void disconnect();
    
    bool isConnected() const;
    QString roomId() const;
    int userCount() const;
    
    QString localUserName() const { return m_localUserName; }
    void setLocalUserName(const QString &name) { m_localUserName = name; }
    
    // Manual sync controls
    void requestFullSync();
    void pushLocalState();

signals:
    void connectionStatusChanged(bool connected);
    void userJoined(const QString &oderId, const QString &userName);
    void userLeft(const QString &oderId);
    void boardSynced();
    void syncReceived(int imagesAdded, int textsAdded);
    void errorOccurred(const QString &error);

private slots:
    void onConnected();
    void onDisconnected();
    void onMessageReceived(const QJsonObject &message);
    void onLocalCursorMoved(const QPointF &pos);
    void onImageAdded(ImageItem *item);
    void onImageChanged(ImageItem *item);
    void onImageRemoved(const QString &id);
    void onTextAdded(TextItem *item);
    void onTextChanged(TextItem *item);
    void onTextRemoved(const QString &id);
    void sendCursorUpdate();

private:
    void handleJoin(const QJsonObject &message);
    void handleLeave(const QJsonObject &message);
    void handleUserList(const QJsonObject &message);
    void handleCursor(const QJsonObject &message);
    void handleImageAdd(const QJsonObject &message);
    void handleImageUpdate(const QJsonObject &message);
    void handleImageRemove(const QJsonObject &message);
    void handleTextAdd(const QJsonObject &message);
    void handleTextUpdate(const QJsonObject &message);
    void handleTextRemove(const QJsonObject &message);
    void handleSync(const QJsonObject &message);
    void handleFullSync(const QJsonObject &message);
    
    void sendImageAdd(ImageItem *item);
    void sendImageUpdate(ImageItem *item);
    void sendImageRemove(const QString &id);
    void sendTextAdd(TextItem *item);
    void sendTextUpdate(TextItem *item);
    void sendTextRemove(const QString &id);
    
    QColor generateUserColor() const;
    
    SyncClient *m_client;
    Board *m_board;
    CanvasScene *m_scene;
    
    QString m_localUserName;
    QColor m_localColor;
    QHash<QString, Collaborator> m_collaborators;
    
    QTimer *m_cursorThrottle;
    QTimer *m_syncTimer;
    QPointF m_pendingCursorPos;
    bool m_hasPendingCursor;
    
    bool m_isSyncing;
    
    static constexpr int CURSOR_THROTTLE_MS = 50;
};

#endif // COLLABMANAGER_H
