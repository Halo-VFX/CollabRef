#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPoint>
#include <QTimer>

class CanvasView;
class CanvasScene;
class TitleBar;
class ToolBar;
class CollabManager;
class Board;
class SyncServer;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void connectToServer(const QString &url, const QString &roomId = "");
    void loadBoard(const QString &filePath);
    void saveBoard(const QString &filePath);

public slots:
    void newBoard();
    void openBoard();
    void saveBoardAs();
    void saveCurrentBoard();
    void hostSession();
    void joinSession();
    void leaveSession();
    void toggleAlwaysOnTop();
    void toggleTransparency();
    void setOpacity(int percent);
    void showAbout();
    void showSettings();

signals:
    void boardChanged();

protected:
    void closeEvent(QCloseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;

private slots:
    void onConnectionStatusChanged(bool connected);
    void onUserJoined(const QString &userId, const QString &userName);
    void onUserLeft(const QString &userId);
    void onBoardSynced();
    void updateWindowTitle();
    void autoSave();
    void onServerClientConnected(const QString &clientId);
    void onServerClientDisconnected(const QString &clientId);
    void autoConnectToServer();
    void tryReconnect();

private:
    void setupUI();
    void setupMenus();
    void setupShortcuts();
    void loadSettings();
    void saveSettings();
    void setFramelessWindow();
    void updateConnectionIndicator();
    QString loadServerConfig();

    CanvasView *m_canvasView;
    CanvasScene *m_canvasScene;
    TitleBar *m_titleBar;
    ToolBar *m_toolBar;
    CollabManager *m_collabManager;
    Board *m_board;
    SyncServer *m_server;  // Built-in server for hosting
    bool m_isHosting;
    QString m_ngrokUrl;  // Store ngrok URL for easy re-copying

    QString m_currentFilePath;
    bool m_isModified;
    bool m_alwaysOnTop;
    bool m_isTransparent;
    int m_opacity;

    // Window dragging
    QPoint m_dragPosition;
    bool m_isDragging;
    
    // Resize handling
    enum ResizeEdge {
        None = 0,
        Left = 1,
        Right = 2,
        Top = 4,
        Bottom = 8,
        TopLeft = Top | Left,
        TopRight = Top | Right,
        BottomLeft = Bottom | Left,
        BottomRight = Bottom | Right
    };
    ResizeEdge m_resizeEdge;
    QPoint m_resizeStartPos;
    QRect m_resizeStartGeometry;

    QTimer *m_autoSaveTimer;
    QTimer *m_reconnectTimer;
    QString m_configuredServerUrl;
    QString m_configuredRoomId;
    
    static constexpr int RESIZE_MARGIN = 8;
};

#endif // MAINWINDOW_H
