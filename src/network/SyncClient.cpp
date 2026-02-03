#include "SyncClient.h"

#include <QJsonDocument>
#include <QUuid>

SyncClient::SyncClient(QObject *parent)
    : QObject(parent)
    , m_socket(new QWebSocket(QString(), QWebSocketProtocol::VersionLatest, this))
    , m_pingTimer(new QTimer(this))
    , m_reconnectTimer(new QTimer(this))
    , m_reconnectAttempts(0)
{
    m_oderId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    
    connect(m_socket, &QWebSocket::connected,
            this, &SyncClient::onConnected);
    connect(m_socket, &QWebSocket::disconnected,
            this, &SyncClient::onDisconnected);
    connect(m_socket, &QWebSocket::textMessageReceived,
            this, &SyncClient::onTextMessageReceived);
    
    // Qt version compatibility for error signal
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    connect(m_socket, &QWebSocket::errorOccurred,
            this, &SyncClient::onError);
#else
    connect(m_socket, QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::error),
            this, &SyncClient::onError);
#endif
    
    connect(m_pingTimer, &QTimer::timeout, this, &SyncClient::sendPing);
    
    connect(m_reconnectTimer, &QTimer::timeout, this, [this]() {
        if (m_reconnectAttempts < MAX_RECONNECT_ATTEMPTS && !m_serverUrl.isEmpty()) {
            m_reconnectAttempts++;
            m_socket->open(QUrl(m_serverUrl));
        } else {
            m_reconnectTimer->stop();
            emit errorOccurred("Failed to reconnect after multiple attempts");
        }
    });
}

SyncClient::~SyncClient()
{
    disconnect();
}

void SyncClient::connectToServer(const QString &url)
{
    m_serverUrl = url;
    m_reconnectAttempts = 0;
    m_socket->open(QUrl(url));
}

void SyncClient::disconnect()
{
    m_pingTimer->stop();
    m_reconnectTimer->stop();
    
    if (m_socket->state() == QAbstractSocket::ConnectedState) {
        leaveRoom();
        m_socket->close();
    }
}

bool SyncClient::isConnected() const
{
    return m_socket->state() == QAbstractSocket::ConnectedState;
}

void SyncClient::sendMessage(const QJsonObject &message)
{
    if (isConnected()) {
        QJsonDocument doc(message);
        m_socket->sendTextMessage(doc.toJson(QJsonDocument::Compact));
    }
}

void SyncClient::joinRoom(const QString &roomId, const QString &userName)
{
    m_roomId = roomId;
    
    QJsonObject message;
    message["type"] = "join";
    message["roomId"] = roomId;
    message["oderId"] = m_oderId;
    message["userName"] = userName;
    
    sendMessage(message);
}

void SyncClient::leaveRoom()
{
    if (!m_roomId.isEmpty()) {
        QJsonObject message;
        message["type"] = "leave";
        message["roomId"] = m_roomId;
        message["oderId"] = m_oderId;
        
        sendMessage(message);
        m_roomId.clear();
    }
}

void SyncClient::onConnected()
{
    m_reconnectAttempts = 0;
    m_reconnectTimer->stop();
    m_pingTimer->start(PING_INTERVAL);
    
    emit connected();
}

void SyncClient::onDisconnected()
{
    m_pingTimer->stop();
    emit disconnected();
    
    // Try to reconnect
    if (!m_serverUrl.isEmpty() && m_reconnectAttempts < MAX_RECONNECT_ATTEMPTS) {
        m_reconnectTimer->start(RECONNECT_INTERVAL);
    }
}

void SyncClient::onTextMessageReceived(const QString &message)
{
    QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8());
    if (doc.isObject()) {
        emit messageReceived(doc.object());
    }
}

void SyncClient::onError(QAbstractSocket::SocketError error)
{
    Q_UNUSED(error)
    emit errorOccurred(m_socket->errorString());
}

void SyncClient::sendPing()
{
    QJsonObject message;
    message["type"] = "ping";
    message["oderId"] = m_oderId;
    sendMessage(message);
}
