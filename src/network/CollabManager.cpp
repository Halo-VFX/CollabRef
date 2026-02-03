#include "CollabManager.h"
#include "SyncClient.h"
#include "data/Board.h"
#include "canvas/CanvasScene.h"
#include "canvas/ImageItem.h"
#include "canvas/TextItem.h"

#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QBuffer>
#include <QRandomGenerator>
#include <QFile>
#include <QDir>
#include <QStandardPaths>
#include <QUuid>

CollabManager::CollabManager(QObject *parent)
    : QObject(parent)
    , m_client(new SyncClient(this))
    , m_board(nullptr)
    , m_scene(nullptr)
    , m_localUserName("User")
    , m_cursorThrottle(new QTimer(this))
    , m_hasPendingCursor(false)
    , m_isSyncing(false)
    , m_syncTimer(new QTimer(this))
{
    m_localColor = generateUserColor();
    
    connect(m_client, &SyncClient::connected, this, &CollabManager::onConnected);
    connect(m_client, &SyncClient::disconnected, this, &CollabManager::onDisconnected);
    connect(m_client, &SyncClient::messageReceived, this, &CollabManager::onMessageReceived);
    connect(m_client, &SyncClient::errorOccurred, this, &CollabManager::errorOccurred);
    
    m_cursorThrottle->setInterval(CURSOR_THROTTLE_MS);
    m_cursorThrottle->setSingleShot(true);
    connect(m_cursorThrottle, &QTimer::timeout, this, &CollabManager::sendCursorUpdate);
    
    // Periodic sync timer - every 5 seconds while connected
    m_syncTimer->setInterval(5000);
    connect(m_syncTimer, &QTimer::timeout, this, &CollabManager::requestFullSync);
}

CollabManager::~CollabManager()
{
    disconnect();
}

void CollabManager::setBoard(Board *board)
{
    if (m_board) {
        QObject::disconnect(m_board, nullptr, this, nullptr);
    }
    
    m_board = board;
}

void CollabManager::setScene(CanvasScene *scene)
{
    if (m_scene) {
        QObject::disconnect(m_scene, nullptr, this, nullptr);
    }
    
    m_scene = scene;
    
    if (m_scene) {
        connect(m_scene, &CanvasScene::localCursorMoved,
                this, &CollabManager::onLocalCursorMoved);
        connect(m_scene, &CanvasScene::imageAdded,
                this, &CollabManager::onImageAdded);
        connect(m_scene, &CanvasScene::imageChanged,
                this, &CollabManager::onImageChanged);
        connect(m_scene, &CanvasScene::imageRemoved,
                this, &CollabManager::onImageRemoved);
        connect(m_scene, &CanvasScene::textAdded,
                this, &CollabManager::onTextAdded);
        connect(m_scene, &CanvasScene::textChanged,
                this, &CollabManager::onTextChanged);
        connect(m_scene, &CanvasScene::textRemoved,
                this, &CollabManager::onTextRemoved);
    }
}

void CollabManager::connectToServer(const QString &url, const QString &roomId)
{
    m_client->connectToServer(url);
    
    // Join room after connection established
    QMetaObject::Connection *conn = new QMetaObject::Connection;
    *conn = connect(m_client, &SyncClient::connected, this, [this, roomId, conn]() {
        m_client->joinRoom(roomId, m_localUserName);
        
        // Push our local state to sync with server
        // Use a longer delay to ensure we've received server state first
        QTimer::singleShot(500, this, &CollabManager::pushLocalState);
        
        // Request full sync again after push to catch any updates from others
        QTimer::singleShot(1500, this, &CollabManager::requestFullSync);
        
        QObject::disconnect(*conn);
        delete conn;
    });
}

void CollabManager::disconnect()
{
    m_syncTimer->stop();
    
    // Remove all remote cursors
    for (const QString &oderId : m_collaborators.keys()) {
        if (m_scene) {
            m_scene->removeRemoteCursor(oderId);
        }
    }
    m_collaborators.clear();
    
    m_client->disconnect();
}

bool CollabManager::isConnected() const
{
    return m_client->isConnected();
}

QString CollabManager::roomId() const
{
    return m_client->roomId();
}

int CollabManager::userCount() const
{
    return m_collaborators.size() + 1; // Include self
}

void CollabManager::onConnected()
{
    m_syncTimer->start();
    emit connectionStatusChanged(true);
}

void CollabManager::onDisconnected()
{
    m_syncTimer->stop();
    emit connectionStatusChanged(false);
}

void CollabManager::onMessageReceived(const QJsonObject &message)
{
    QString type = message["type"].toString();
    
    if (type == "join") {
        handleJoin(message);
    } else if (type == "leave") {
        handleLeave(message);
    } else if (type == "userList") {
        handleUserList(message);
    } else if (type == "cursor") {
        handleCursor(message);
    } else if (type == "imageAdd") {
        handleImageAdd(message);
    } else if (type == "imageUpdate") {
        handleImageUpdate(message);
    } else if (type == "imageRemove") {
        handleImageRemove(message);
    } else if (type == "textAdd") {
        handleTextAdd(message);
    } else if (type == "textUpdate") {
        handleTextUpdate(message);
    } else if (type == "textRemove") {
        handleTextRemove(message);
    } else if (type == "sync") {
        handleSync(message);
    } else if (type == "fullSync") {
        handleFullSync(message);
    }
}

void CollabManager::onLocalCursorMoved(const QPointF &pos)
{
    m_pendingCursorPos = pos;
    m_hasPendingCursor = true;
    
    if (!m_cursorThrottle->isActive()) {
        sendCursorUpdate();
        m_cursorThrottle->start();
    }
}

void CollabManager::onImageAdded(ImageItem *item)
{
    if (!m_isSyncing && isConnected()) {
        sendImageAdd(item);
    }
}

void CollabManager::onImageChanged(ImageItem *item)
{
    if (!m_isSyncing && isConnected()) {
        sendImageUpdate(item);
    }
}

void CollabManager::onImageRemoved(const QString &id)
{
    if (!m_isSyncing && isConnected()) {
        sendImageRemove(id);
    }
}

void CollabManager::onTextAdded(TextItem *item)
{
    if (!m_isSyncing && isConnected()) {
        sendTextAdd(item);
    }
}

void CollabManager::onTextChanged(TextItem *item)
{
    if (!m_isSyncing && isConnected()) {
        sendTextUpdate(item);
    }
}

void CollabManager::onTextRemoved(const QString &id)
{
    if (!m_isSyncing && isConnected()) {
        sendTextRemove(id);
    }
}

void CollabManager::sendCursorUpdate()
{
    if (m_hasPendingCursor && isConnected()) {
        QJsonObject message;
        message["type"] = "cursor";
        message["oderId"] = m_client->oderId();
        message["x"] = m_pendingCursorPos.x();
        message["y"] = m_pendingCursorPos.y();
        message["color"] = m_localColor.name();
        
        m_client->sendMessage(message);
        m_hasPendingCursor = false;
    }
}

void CollabManager::handleJoin(const QJsonObject &message)
{
    QString oderId = message["oderId"].toString();
    
    if (oderId == m_client->oderId()) {
        // It's us joining - push our local state and get merged sync back
        pushLocalState();
        return;
    }
    
    QString userName = message["userName"].toString();
    
    Collaborator collab;
    collab.oderId = oderId;
    collab.userName = userName;
    collab.color = QColor(message["color"].toString());
    if (!collab.color.isValid()) {
        collab.color = generateUserColor();
    }
    collab.isActive = true;
    
    m_collaborators.insert(oderId, collab);
    emit userJoined(oderId, userName);
}

void CollabManager::handleLeave(const QJsonObject &message)
{
    QString oderId = message["oderId"].toString();
    
    if (m_collaborators.contains(oderId)) {
        m_collaborators.remove(oderId);
        
        if (m_scene) {
            m_scene->removeRemoteCursor(oderId);
        }
        
        emit userLeft(oderId);
    }
}

void CollabManager::handleUserList(const QJsonObject &message)
{
    QJsonArray users = message["users"].toArray();
    
    for (const QJsonValue &value : users) {
        QJsonObject user = value.toObject();
        QString oderId = user["oderId"].toString();
        
        if (oderId == m_client->oderId()) continue;
        
        Collaborator collab;
        collab.oderId = oderId;
        collab.userName = user["userName"].toString();
        collab.color = QColor(user["color"].toString());
        if (!collab.color.isValid()) {
            collab.color = generateUserColor();
        }
        collab.isActive = true;
        
        m_collaborators.insert(oderId, collab);
    }
}

void CollabManager::handleCursor(const QJsonObject &message)
{
    QString oderId = message["oderId"].toString();
    
    if (oderId == m_client->oderId()) return;
    
    QPointF pos(message["x"].toDouble(), message["y"].toDouble());
    QColor color(message["color"].toString());
    
    if (m_collaborators.contains(oderId)) {
        m_collaborators[oderId].cursorPos = pos;
        
        if (m_scene) {
            m_scene->updateRemoteCursor(oderId, m_collaborators[oderId].userName,
                                       pos, m_collaborators[oderId].color);
        }
    }
}

void CollabManager::handleImageAdd(const QJsonObject &message)
{
    QString senderId = message["oderId"].toString();
    if (senderId == m_client->oderId()) return;
    
    QString imageId = message["imageId"].toString();
    
    // Check if we already have this image
    if (m_scene && m_scene->findImageItem(imageId)) return;
    
    // Decode image data
    QByteArray imageData = QByteArray::fromBase64(message["imageData"].toString().toUtf8());
    
    QPointF pos(message["x"].toDouble(), message["y"].toDouble());
    qreal rotation = message["rotation"].toDouble();
    qreal scale = message["scale"].toDouble(1.0);
    
    m_isSyncing = true;
    
    // Check if this is a GIF
    bool isGif = message["isGif"].toBool(false);
    
    if (isGif && m_scene) {
        // Save GIF to temp file to enable animation
        QString tempDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
        QString tempPath = tempDir + "/collabref_" + QUuid::createUuid().toString(QUuid::WithoutBraces) + ".gif";
        
        QFile tempFile(tempPath);
        if (tempFile.open(QIODevice::WriteOnly)) {
            tempFile.write(imageData);
            tempFile.close();
            
            // Create ImageItem with file path to enable animation
            m_scene->addImageItemFromFile(imageId, tempPath, pos, rotation, scale);
        }
    } else if (m_scene) {
        // Regular image
        QImage image;
        image.loadFromData(imageData);
        
        if (!image.isNull()) {
            m_scene->addImageItem(imageId, image, pos, rotation, scale);
        }
    }
    
    m_isSyncing = false;
}

void CollabManager::handleImageUpdate(const QJsonObject &message)
{
    QString senderId = message["oderId"].toString();
    if (senderId == m_client->oderId()) return;
    
    QString imageId = message["imageId"].toString();
    
    if (!m_scene) return;
    
    ImageItem *item = m_scene->findImageItem(imageId);
    if (!item) return;
    
    m_isSyncing = true;
    
    if (message.contains("x") && message.contains("y")) {
        item->setPos(QPointF(message["x"].toDouble(), message["y"].toDouble()));
    }
    if (message.contains("rotation")) {
        item->setRotation(message["rotation"].toDouble());
    }
    if (message.contains("scale")) {
        item->setScale(message["scale"].toDouble());
    }
    if (message.contains("zIndex")) {
        item->setZValue(message["zIndex"].toDouble());
    }
    
    m_isSyncing = false;
}

void CollabManager::handleImageRemove(const QJsonObject &message)
{
    QString senderId = message["oderId"].toString();
    if (senderId == m_client->oderId()) return;
    
    QString imageId = message["imageId"].toString();
    
    m_isSyncing = true;
    if (m_scene) {
        m_scene->removeImageItem(imageId);
    }
    m_isSyncing = false;
}

void CollabManager::handleSync(const QJsonObject &message)
{
    // Single item sync
    handleImageUpdate(message);
}

void CollabManager::handleFullSync(const QJsonObject &message)
{
    if (!m_scene) return;
    
    m_isSyncing = true;
    
    int addedImages = 0;
    int addedTexts = 0;
    
    // Sync images
    QJsonArray images = message["images"].toArray();
    
    for (const QJsonValue &value : images) {
        QJsonObject imgObj = value.toObject();
        
        QString imageId = imgObj["imageId"].toString();
        if (imageId.isEmpty()) continue;
        
        // Skip if we already have this image
        if (m_scene->findImageItem(imageId)) continue;
        
        QString imageDataStr = imgObj["imageData"].toString();
        if (imageDataStr.isEmpty()) continue;
        
        QByteArray imageData = QByteArray::fromBase64(imageDataStr.toUtf8());
        if (imageData.isEmpty()) continue;
        
        QPointF pos(imgObj["x"].toDouble(), imgObj["y"].toDouble());
        qreal rotation = imgObj["rotation"].toDouble();
        qreal scale = imgObj["scale"].toDouble(1.0);
        
        // Check if this is a GIF
        bool isGif = imgObj["isGif"].toBool(false);
        
        if (isGif) {
            // Save GIF to temp file to enable animation
            QString tempDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
            QString tempPath = tempDir + "/collabref_" + QUuid::createUuid().toString(QUuid::WithoutBraces) + ".gif";
            
            QFile tempFile(tempPath);
            if (tempFile.open(QIODevice::WriteOnly)) {
                tempFile.write(imageData);
                tempFile.close();
                m_scene->addImageItemFromFile(imageId, tempPath, pos, rotation, scale);
                addedImages++;
            }
        } else {
            QImage image;
            image.loadFromData(imageData);
            
            if (!image.isNull()) {
                m_scene->addImageItem(imageId, image, pos, rotation, scale);
                addedImages++;
            }
        }
    }
    
    // Sync text items
    QJsonArray texts = message["texts"].toArray();
    
    for (const QJsonValue &value : texts) {
        QJsonObject txtObj = value.toObject();
        
        QString textId = txtObj["textId"].toString();
        if (textId.isEmpty()) continue;
        
        // Skip if we already have this text
        if (m_scene->findTextItem(textId)) continue;
        
        QString text = txtObj["text"].toString();
        QPointF pos(txtObj["x"].toDouble(), txtObj["y"].toDouble());
        qreal rotation = txtObj["rotation"].toDouble();
        
        m_scene->addTextItem(textId, text, pos, rotation);
        addedTexts++;
    }
    
    m_isSyncing = false;
    
    // Emit what was synced for debugging
    emit syncReceived(addedImages, addedTexts);
    emit boardSynced();
}

void CollabManager::requestFullSync()
{
    QJsonObject message;
    message["type"] = "requestSync";
    message["oderId"] = m_client->oderId();
    
    m_client->sendMessage(message);
}

void CollabManager::pushLocalState()
{
    if (!m_scene || !isConnected()) {
        return;
    }
    
    QJsonObject message;
    message["type"] = "pushSync";
    message["oderId"] = m_client->oderId();
    
    // Gather local images
    QJsonArray images;
    for (ImageItem *item : m_scene->imageItems()) {
        QJsonObject imgObj;
        imgObj["imageId"] = item->id();
        imgObj["x"] = item->pos().x();
        imgObj["y"] = item->pos().y();
        imgObj["rotation"] = item->rotation();
        imgObj["scale"] = item->scale();
        imgObj["zIndex"] = item->zValue();
        
        // Check if it's an animated GIF
        QString sourcePath = item->sourcePath();
        if (!sourcePath.isEmpty() && sourcePath.toLower().endsWith(".gif")) {
            QFile file(sourcePath);
            if (file.open(QIODevice::ReadOnly)) {
                QByteArray gifData = file.readAll();
                file.close();
                imgObj["imageData"] = QString::fromUtf8(gifData.toBase64());
                imgObj["isGif"] = true;
            }
        } else {
            QByteArray imageData;
            QBuffer buffer(&imageData);
            buffer.open(QIODevice::WriteOnly);
            item->image().save(&buffer, "PNG");
            imgObj["imageData"] = QString::fromUtf8(imageData.toBase64());
            imgObj["isGif"] = false;
        }
        
        images.append(imgObj);
    }
    message["images"] = images;
    
    // Gather local texts
    QJsonArray texts;
    for (TextItem *item : m_scene->textItems()) {
        QJsonObject txtObj;
        txtObj["textId"] = item->id();
        txtObj["text"] = item->text();
        txtObj["x"] = item->pos().x();
        txtObj["y"] = item->pos().y();
        txtObj["rotation"] = item->rotation();
        texts.append(txtObj);
    }
    message["texts"] = texts;
    
    m_client->sendMessage(message);
}

void CollabManager::sendImageAdd(ImageItem *item)
{
    if (!item) return;
    
    QJsonObject message;
    message["type"] = "imageAdd";
    message["oderId"] = m_client->oderId();
    message["imageId"] = item->id();
    message["x"] = item->pos().x();
    message["y"] = item->pos().y();
    message["rotation"] = item->rotation();
    message["scale"] = item->scale();
    message["zIndex"] = item->zValue();
    
    // Check if it's an animated GIF
    QString sourcePath = item->sourcePath();
    if (!sourcePath.isEmpty() && sourcePath.toLower().endsWith(".gif")) {
        // Send raw GIF file data to preserve animation
        QFile file(sourcePath);
        if (file.open(QIODevice::ReadOnly)) {
            QByteArray gifData = file.readAll();
            file.close();
            message["imageData"] = QString::fromUtf8(gifData.toBase64());
            message["isGif"] = true;
        } else {
            // Fallback to static image
            QByteArray imageData;
            QBuffer buffer(&imageData);
            buffer.open(QIODevice::WriteOnly);
            item->image().save(&buffer, "PNG");
            message["imageData"] = QString::fromUtf8(imageData.toBase64());
            message["isGif"] = false;
        }
    } else {
        // Regular image - encode as PNG
        QByteArray imageData;
        QBuffer buffer(&imageData);
        buffer.open(QIODevice::WriteOnly);
        item->image().save(&buffer, "PNG");
        message["imageData"] = QString::fromUtf8(imageData.toBase64());
        message["isGif"] = false;
    }
    
    m_client->sendMessage(message);
}

void CollabManager::sendImageUpdate(ImageItem *item)
{
    if (!item) return;
    
    QJsonObject message;
    message["type"] = "imageUpdate";
    message["oderId"] = m_client->oderId();
    message["imageId"] = item->id();
    message["x"] = item->pos().x();
    message["y"] = item->pos().y();
    message["rotation"] = item->rotation();
    message["scale"] = item->scale();
    message["zIndex"] = item->zValue();
    
    m_client->sendMessage(message);
}

void CollabManager::sendImageRemove(const QString &id)
{
    QJsonObject message;
    message["type"] = "imageRemove";
    message["oderId"] = m_client->oderId();
    message["imageId"] = id;
    
    m_client->sendMessage(message);
}

void CollabManager::sendTextAdd(TextItem *item)
{
    QJsonObject message;
    message["type"] = "textAdd";
    message["oderId"] = m_client->oderId();
    message["textId"] = item->id();
    message["text"] = item->text();
    message["x"] = item->pos().x();
    message["y"] = item->pos().y();
    message["rotation"] = item->rotation();
    message["fontFamily"] = item->textFont().family();
    message["fontSize"] = item->textFont().pointSize();
    message["textColor"] = item->textColor().name();
    
    m_client->sendMessage(message);
}

void CollabManager::sendTextUpdate(TextItem *item)
{
    QJsonObject message;
    message["type"] = "textUpdate";
    message["oderId"] = m_client->oderId();
    message["textId"] = item->id();
    message["text"] = item->text();
    message["x"] = item->pos().x();
    message["y"] = item->pos().y();
    message["rotation"] = item->rotation();
    
    m_client->sendMessage(message);
}

void CollabManager::sendTextRemove(const QString &id)
{
    QJsonObject message;
    message["type"] = "textRemove";
    message["oderId"] = m_client->oderId();
    message["textId"] = id;
    
    m_client->sendMessage(message);
}

void CollabManager::handleTextAdd(const QJsonObject &message)
{
    if (!m_scene) return;
    
    QString senderId = message["oderId"].toString();
    if (senderId == m_client->oderId()) return; // Ignore own messages
    
    QString textId = message["textId"].toString();
    QString text = message["text"].toString();
    qreal x = message["x"].toDouble();
    qreal y = message["y"].toDouble();
    qreal rotation = message["rotation"].toDouble();
    
    // Check if text already exists
    if (m_scene->findTextItem(textId)) return;
    
    m_isSyncing = true;
    m_scene->addTextItem(textId, text, QPointF(x, y), rotation);
    m_isSyncing = false;
}

void CollabManager::handleTextUpdate(const QJsonObject &message)
{
    if (!m_scene) return;
    
    QString senderId = message["oderId"].toString();
    if (senderId == m_client->oderId()) return;
    
    QString textId = message["textId"].toString();
    TextItem *item = m_scene->findTextItem(textId);
    if (!item) return;
    
    m_isSyncing = true;
    
    if (message.contains("text")) {
        item->setText(message["text"].toString());
    }
    if (message.contains("x") && message.contains("y")) {
        item->setPos(message["x"].toDouble(), message["y"].toDouble());
    }
    if (message.contains("rotation")) {
        item->setRotation(message["rotation"].toDouble());
    }
    
    m_isSyncing = false;
}

void CollabManager::handleTextRemove(const QJsonObject &message)
{
    if (!m_scene) return;
    
    QString senderId = message["oderId"].toString();
    if (senderId == m_client->oderId()) return;
    
    QString textId = message["textId"].toString();
    
    m_isSyncing = true;
    m_scene->removeTextItem(textId);
    m_isSyncing = false;
}

QColor CollabManager::generateUserColor() const
{
    static const QList<QColor> colors = {
        QColor(231, 76, 60),   // Red
        QColor(46, 204, 113),  // Green
        QColor(52, 152, 219),  // Blue
        QColor(155, 89, 182),  // Purple
        QColor(241, 196, 15),  // Yellow
        QColor(230, 126, 34),  // Orange
        QColor(26, 188, 156),  // Teal
        QColor(236, 112, 99),  // Light Red
    };
    
    return colors[QRandomGenerator::global()->bounded(colors.size())];
}
