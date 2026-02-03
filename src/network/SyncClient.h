#ifndef SYNCCLIENT_H
#define SYNCCLIENT_H

#include <QObject>
#include <QWebSocket>
#include <QTimer>
#include <QJsonObject>

class SyncClient : public QObject
{
    Q_OBJECT

public:
    explicit SyncClient(QObject *parent = nullptr);
    ~SyncClient();

    void connectToServer(const QString &url);
    void disconnect();
    
    bool isConnected() const;
    
    void sendMessage(const QJsonObject &message);
    void joinRoom(const QString &roomId, const QString &userName);
    void leaveRoom();
    
    QString oderId() const { return m_oderId; }
    QString roomId() const { return m_roomId; }

signals:
    void connected();
    void disconnected();
    void messageReceived(const QJsonObject &message);
    void errorOccurred(const QString &error);

private slots:
    void onConnected();
    void onDisconnected();
    void onTextMessageReceived(const QString &message);
    void onError(QAbstractSocket::SocketError error);
    void sendPing();

private:
    QWebSocket *m_socket;
    QString m_serverUrl;
    QString m_roomId;
    QString m_oderId;
    QTimer *m_pingTimer;
    QTimer *m_reconnectTimer;
    int m_reconnectAttempts;
    
    static constexpr int PING_INTERVAL = 30000;
    static constexpr int RECONNECT_INTERVAL = 5000;
    static constexpr int MAX_RECONNECT_ATTEMPTS = 10;
};

#endif // SYNCCLIENT_H
