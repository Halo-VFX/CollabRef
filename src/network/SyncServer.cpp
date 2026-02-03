#include "SyncServer.h"
#include <QNetworkInterface>
#include <QUuid>
#include <QJsonArray>
#include <QFile>
#include <QDir>
#include <QStandardPaths>
#include <QDateTime>

SyncServer::SyncServer(QObject *parent)
    : QObject(parent)
    , m_server(nullptr)
    , m_roomId(QUuid::createUuid().toString(QUuid::WithoutBraces).left(8))
    , m_saveTimer(new QTimer(this))
{
    // Auto-save every 30 seconds
    connect(m_saveTimer, &QTimer::timeout, this, &SyncServer::saveState);
    
    // Default save location
    QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dataDir);
    m_saveFilePath = dataDir + "/shared_board.json";
}

SyncServer::~SyncServer()
{
    stop();
}

bool SyncServer::start(quint16 port)
{
    if (m_server) {
        stop();
    }

    m_server = new QWebSocketServer("CollabRef Server", 
                                     QWebSocketServer::NonSecureMode, this);
    
    if (m_server->listen(QHostAddress::Any, port)) {
        connect(m_server, &QWebSocketServer::newConnection,
                this, &SyncServer::onNewConnection);
        
        // Load existing state from disk
        loadState();
        
        // Start auto-save timer
        m_saveTimer->start(30000);  // 30 seconds
        
        emit serverStarted(m_server->serverPort());
        return true;
    } else {
        QString error = m_server->errorString();
        delete m_server;
        m_server = nullptr;
        emit errorOccurred(error);
        return false;
    }
}

void SyncServer::stop()
{
    if (m_server) {
        // Save state before stopping
        saveState();
        m_saveTimer->stop();
        
        // Close all client connections
        for (QWebSocket *client : m_clients) {
            client->close();
        }
        m_clients.clear();
        m_clientIds.clear();
        m_clientsById.clear();
        
        // Don't clear state - keep it for next start
        // m_boardState = QJsonArray();
        // m_textState = QJsonArray();
        
        m_server->close();
        delete m_server;
        m_server = nullptr;
        emit serverStopped();
    }
}

bool SyncServer::isRunning() const
{
    return m_server && m_server->isListening();
}

quint16 SyncServer::port() const
{
    return m_server ? m_server->serverPort() : 0;
}

QString SyncServer::localAddress() const
{
    // Find the best local IP address to share
    QList<QHostAddress> addresses = QNetworkInterface::allAddresses();
    
    for (const QHostAddress &addr : addresses) {
        if (addr.protocol() == QAbstractSocket::IPv4Protocol && 
            !addr.isLoopback()) {
            QString ip = addr.toString();
            // Prefer 192.168.x.x or 10.x.x.x addresses
            if (ip.startsWith("192.168.") || ip.startsWith("10.")) {
                return ip;
            }
        }
    }
    
    // Fallback: return first non-loopback IPv4
    for (const QHostAddress &addr : addresses) {
        if (addr.protocol() == QAbstractSocket::IPv4Protocol && 
            !addr.isLoopback()) {
            return addr.toString();
        }
    }
    
    return "127.0.0.1";
}

void SyncServer::onNewConnection()
{
    QWebSocket *client = m_server->nextPendingConnection();
    if (!client) return;

    QString clientId = QUuid::createUuid().toString(QUuid::WithoutBraces).left(8);
    
    m_clients.append(client);
    m_clientIds.insert(client, clientId);
    m_clientsById.insert(clientId, client);

    connect(client, &QWebSocket::textMessageReceived,
            this, &SyncServer::onTextMessageReceived);
    connect(client, &QWebSocket::disconnected,
            this, &SyncServer::onClientDisconnected);

    emit clientConnected(clientId);

    // Send current board state to new client
    if (!m_boardState.isEmpty() || !m_textState.isEmpty()) {
        QJsonObject syncMsg;
        syncMsg["type"] = "fullSync";
        syncMsg["images"] = m_boardState;
        syncMsg["texts"] = m_textState;
        sendToClient(clientId, syncMsg);
    }
}

void SyncServer::onClientDisconnected()
{
    QWebSocket *client = qobject_cast<QWebSocket*>(sender());
    if (!client) return;

    QString clientId = m_clientIds.value(client);
    
    m_clients.removeAll(client);
    m_clientIds.remove(client);
    m_clientsById.remove(clientId);
    
    client->deleteLater();

    emit clientDisconnected(clientId);

    // Notify other clients
    QJsonObject leaveMsg;
    leaveMsg["type"] = "leave";
    leaveMsg["userId"] = clientId;
    broadcast(leaveMsg, clientId);
}

void SyncServer::onTextMessageReceived(const QString &message)
{
    QWebSocket *client = qobject_cast<QWebSocket*>(sender());
    if (!client) return;

    QString clientId = m_clientIds.value(client);

    QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8());
    if (!doc.isObject()) return;

    QJsonObject msg = doc.object();
    QString type = msg["type"].toString();

    // Handle different message types
    if (type == "join") {
        // Broadcast join to others
        msg["userId"] = clientId;
        broadcast(msg, clientId);
        
        // Send user list to the joining client
        QJsonObject userListMsg;
        userListMsg["type"] = "userList";
        QJsonArray users;
        for (const QString &id : m_clientsById.keys()) {
            if (id != clientId) {
                QJsonObject user;
                user["userId"] = id;
                users.append(user);
            }
        }
        userListMsg["users"] = users;
        sendToClient(clientId, userListMsg);
    }
    else if (type == "cursor") {
        // Forward cursor updates to others
        msg["userId"] = clientId;
        broadcast(msg, clientId);
    }
    else if (type == "imageAdd") {
        // Check if image already exists (from another client's sync)
        QString imageId = msg["imageId"].toString();
        bool exists = false;
        for (int i = 0; i < m_boardState.count(); i++) {
            QJsonObject img = m_boardState[i].toObject();
            if (img["imageId"].toString() == imageId) {
                exists = true;
                break;
            }
        }
        if (!exists) {
            m_boardState.append(msg);
            saveState();  // Save immediately on add
        }
        msg["userId"] = clientId;
        broadcast(msg, clientId);
    }
    else if (type == "imageUpdate") {
        // Update stored state and broadcast
        QString imageId = msg["imageId"].toString();
        for (int i = 0; i < m_boardState.count(); i++) {
            QJsonObject img = m_boardState[i].toObject();
            if (img["imageId"].toString() == imageId) {
                // Merge update into stored image
                for (auto it = msg.begin(); it != msg.end(); ++it) {
                    img[it.key()] = it.value();
                }
                m_boardState[i] = img;
                break;
            }
        }
        msg["userId"] = clientId;
        broadcast(msg, clientId);
    }
    else if (type == "imageRemove") {
        // Remove from stored state and broadcast
        QString imageId = msg["imageId"].toString();
        for (int i = 0; i < m_boardState.count(); i++) {
            QJsonObject img = m_boardState[i].toObject();
            if (img["imageId"].toString() == imageId) {
                m_boardState.removeAt(i);
                saveState();  // Save immediately on delete
                break;
            }
        }
        msg["userId"] = clientId;
        broadcast(msg, clientId);
    }
    else if (type == "textAdd") {
        // Check if text already exists
        QString textId = msg["textId"].toString();
        bool exists = false;
        for (int i = 0; i < m_textState.count(); i++) {
            QJsonObject txt = m_textState[i].toObject();
            if (txt["textId"].toString() == textId) {
                exists = true;
                break;
            }
        }
        if (!exists) {
            m_textState.append(msg);
            saveState();  // Save immediately on add
        }
        msg["userId"] = clientId;
        broadcast(msg, clientId);
    }
    else if (type == "textUpdate") {
        QString textId = msg["textId"].toString();
        for (int i = 0; i < m_textState.count(); i++) {
            QJsonObject txt = m_textState[i].toObject();
            if (txt["textId"].toString() == textId) {
                for (auto it = msg.begin(); it != msg.end(); ++it) {
                    txt[it.key()] = it.value();
                }
                m_textState[i] = txt;
                break;
            }
        }
        msg["userId"] = clientId;
        broadcast(msg, clientId);
    }
    else if (type == "textRemove") {
        QString textId = msg["textId"].toString();
        for (int i = 0; i < m_textState.count(); i++) {
            QJsonObject txt = m_textState[i].toObject();
            if (txt["textId"].toString() == textId) {
                m_textState.removeAt(i);
                saveState();  // Save immediately on delete
                break;
            }
        }
        msg["userId"] = clientId;
        broadcast(msg, clientId);
    }
    else if (type == "requestSync") {
        // Send full board state
        QJsonObject syncMsg;
        syncMsg["type"] = "fullSync";
        syncMsg["images"] = m_boardState;
        syncMsg["texts"] = m_textState;
        sendToClient(clientId, syncMsg);
    }
    else if (type == "pushSync") {
        // Client is pushing their local state (after reconnect)
        // Merge with existing state
        QJsonArray clientImages = msg["images"].toArray();
        QJsonArray clientTexts = msg["texts"].toArray();
        
        bool stateChanged = false;
        
        // Merge images
        for (const QJsonValue &val : clientImages) {
            QJsonObject img = val.toObject();
            QString imageId = img["imageId"].toString();
            bool exists = false;
            for (int i = 0; i < m_boardState.count(); i++) {
                if (m_boardState[i].toObject()["imageId"].toString() == imageId) {
                    exists = true;
                    break;
                }
            }
            if (!exists) {
                m_boardState.append(img);
                stateChanged = true;
            }
        }
        
        // Merge texts
        for (const QJsonValue &val : clientTexts) {
            QJsonObject txt = val.toObject();
            QString textId = txt["textId"].toString();
            bool exists = false;
            for (int i = 0; i < m_textState.count(); i++) {
                if (m_textState[i].toObject()["textId"].toString() == textId) {
                    exists = true;
                    break;
                }
            }
            if (!exists) {
                m_textState.append(txt);
                stateChanged = true;
            }
        }
        
        // Always send full state to the pushing client
        QJsonObject syncMsg;
        syncMsg["type"] = "fullSync";
        syncMsg["images"] = m_boardState;
        syncMsg["texts"] = m_textState;
        sendToClient(clientId, syncMsg);
        
        // If state changed, broadcast full state to ALL other clients too
        // This ensures everyone gets synced regardless of timing
        if (stateChanged) {
            broadcast(syncMsg, clientId);
        }
    }
    else {
        // Forward other messages
        msg["userId"] = clientId;
        broadcast(msg, clientId);
    }

    emit messageReceived(clientId, msg);
}

void SyncServer::broadcast(const QJsonObject &message, const QString &excludeClientId)
{
    QByteArray data = QJsonDocument(message).toJson(QJsonDocument::Compact);
    
    for (QWebSocket *client : m_clients) {
        QString clientId = m_clientIds.value(client);
        if (clientId != excludeClientId) {
            client->sendTextMessage(QString::fromUtf8(data));
        }
    }
}

void SyncServer::sendToClient(const QString &clientId, const QJsonObject &message)
{
    QWebSocket *client = m_clientsById.value(clientId);
    if (client) {
        QByteArray data = QJsonDocument(message).toJson(QJsonDocument::Compact);
        client->sendTextMessage(QString::fromUtf8(data));
    }
}

void SyncServer::setSaveFile(const QString &path)
{
    m_saveFilePath = path;
}

void SyncServer::saveState()
{
    if (m_saveFilePath.isEmpty()) return;
    if (m_boardState.isEmpty() && m_textState.isEmpty()) return;
    
    QJsonObject root;
    root["images"] = m_boardState;
    root["texts"] = m_textState;
    root["savedAt"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    
    QFile file(m_saveFilePath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(root).toJson());
        file.close();
    }
}

void SyncServer::loadState()
{
    if (m_saveFilePath.isEmpty()) return;
    
    QFile file(m_saveFilePath);
    if (!file.exists()) return;
    
    if (file.open(QIODevice::ReadOnly)) {
        QByteArray data = file.readAll();
        file.close();
        
        QJsonDocument doc = QJsonDocument::fromJson(data);
        if (doc.isObject()) {
            QJsonObject root = doc.object();
            m_boardState = root["images"].toArray();
            m_textState = root["texts"].toArray();
        }
    }
}
