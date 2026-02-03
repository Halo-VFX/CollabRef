#ifndef SYNCSERVER_H
#define SYNCSERVER_H

#include <QObject>
#include <QWebSocketServer>
#include <QWebSocket>
#include <QList>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QTimer>

class SyncServer : public QObject
{
    Q_OBJECT

public:
    explicit SyncServer(QObject *parent = nullptr);
    ~SyncServer();

    bool start(quint16 port = 8080);
    void stop();
    bool isRunning() const;
    quint16 port() const;
    QString localAddress() const;
    int clientCount() const { return m_clients.count(); }
    
    void setSaveFile(const QString &path);
    void saveState();
    void loadState();

signals:
    void clientConnected(const QString &clientId);
    void clientDisconnected(const QString &clientId);
    void messageReceived(const QString &clientId, const QJsonObject &message);
    void serverStarted(quint16 port);
    void serverStopped();
    void errorOccurred(const QString &error);

public slots:
    void broadcast(const QJsonObject &message, const QString &excludeClientId = QString());
    void sendToClient(const QString &clientId, const QJsonObject &message);

private slots:
    void onNewConnection();
    void onClientDisconnected();
    void onTextMessageReceived(const QString &message);

private:
    QWebSocketServer *m_server;
    QList<QWebSocket*> m_clients;
    QHash<QWebSocket*, QString> m_clientIds;
    QHash<QString, QWebSocket*> m_clientsById;
    
    // Room state for syncing
    QJsonArray m_boardState;  // Images
    QJsonArray m_textState;   // Text items
    QString m_roomId;
    
    // Persistence
    QString m_saveFilePath;
    QTimer *m_saveTimer;
};

#endif // SYNCSERVER_H
