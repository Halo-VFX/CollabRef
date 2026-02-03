#include "MainWindow.h"
#include "canvas/CanvasView.h"
#include "canvas/CanvasScene.h"
#include "canvas/ImageItem.h"
#include "canvas/TextItem.h"
#include "network/CollabManager.h"
#include "network/SyncServer.h"
#include "data/Board.h"
#include "data/BoardSerializer.h"
#include "ui/TitleBar.h"
#include "ui/ToolBar.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QInputDialog>
#include <QClipboard>
#include <QMimeData>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QContextMenuEvent>
#include <QMenu>
#include <QAction>
#include <QPushButton>
#include <QAbstractButton>
#include <QSettings>
#include <QStandardPaths>
#include <QApplication>
#include <QCoreApplication>
#include <QScreen>
#include <QCloseEvent>
#include <QShortcut>
#include <QTimer>
#include <QUuid>
#include <QImageReader>
#include <QFileInfo>
#include <QFile>
#include <QDir>
#include <QTextStream>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QEventLoop>
#include <QProcess>
#include <QThread>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_canvasView(nullptr)
    , m_canvasScene(nullptr)
    , m_titleBar(nullptr)
    , m_toolBar(nullptr)
    , m_collabManager(nullptr)
    , m_board(nullptr)
    , m_server(nullptr)
    , m_isHosting(false)
    , m_isModified(false)
    , m_alwaysOnTop(false)
    , m_isTransparent(false)
    , m_opacity(100)
    , m_isDragging(false)
    , m_resizeEdge(None)
    , m_autoSaveTimer(nullptr)
{
    setupUI();
    setupMenus();
    setupShortcuts();
    loadSettings();
    setFramelessWindow();
    
    // Create new empty board
    m_board = new Board(this);
    m_canvasScene->setBoard(m_board);
    
    // Setup collaboration manager
    m_collabManager = new CollabManager(this);
    m_collabManager->setBoard(m_board);
    m_collabManager->setScene(m_canvasScene);
    
    connect(m_collabManager, &CollabManager::connectionStatusChanged,
            this, &MainWindow::onConnectionStatusChanged);
    connect(m_collabManager, &CollabManager::userJoined,
            this, &MainWindow::onUserJoined);
    connect(m_collabManager, &CollabManager::userLeft,
            this, &MainWindow::onUserLeft);
    connect(m_collabManager, &CollabManager::boardSynced,
            this, &MainWindow::onBoardSynced);
    connect(m_collabManager, &CollabManager::syncReceived,
            this, [this](int images, int texts) {
                if (images > 0 || texts > 0) {
                    m_titleBar->showNotification(
                        QString("Synced: %1 images, %2 texts").arg(images).arg(texts));
                }
            });
    
    // Setup built-in server
    m_server = new SyncServer(this);
    connect(m_server, &SyncServer::clientConnected,
            this, &MainWindow::onServerClientConnected);
    connect(m_server, &SyncServer::clientDisconnected,
            this, &MainWindow::onServerClientDisconnected);
    
    // Auto-save timer
    m_autoSaveTimer = new QTimer(this);
    m_autoSaveTimer->setInterval(60000); // 1 minute
    connect(m_autoSaveTimer, &QTimer::timeout, this, &MainWindow::autoSave);
    
    setAcceptDrops(true);
    updateWindowTitle();
    
    // Auto-connect to server if configured
    QTimer::singleShot(500, this, &MainWindow::autoConnectToServer);
}

MainWindow::~MainWindow()
{
    saveSettings();
}

void MainWindow::setupUI()
{
    // Central widget
    QWidget *centralWidget = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    
    // Title bar
    m_titleBar = new TitleBar(this);
    connect(m_titleBar, &TitleBar::minimizeClicked, this, &QMainWindow::showMinimized);
    connect(m_titleBar, &TitleBar::maximizeClicked, this, [this]() {
        if (isMaximized())
            showNormal();
        else
            showMaximized();
    });
    connect(m_titleBar, &TitleBar::closeClicked, this, &QMainWindow::close);
    mainLayout->addWidget(m_titleBar);
    
    // Tool bar
    m_toolBar = new ToolBar(this);
    mainLayout->addWidget(m_toolBar);
    
    // Canvas
    m_canvasScene = new CanvasScene(this);
    m_canvasView = new CanvasView(m_canvasScene, this);
    mainLayout->addWidget(m_canvasView, 1);
    
    // Connect toolbar to canvas
    connect(m_toolBar, &ToolBar::zoomInClicked, m_canvasView, &CanvasView::zoomIn);
    connect(m_toolBar, &ToolBar::zoomOutClicked, m_canvasView, &CanvasView::zoomOut);
    connect(m_toolBar, &ToolBar::fitAllClicked, m_canvasView, &CanvasView::fitAll);
    connect(m_toolBar, &ToolBar::resetViewClicked, m_canvasView, &CanvasView::resetView);
    connect(m_toolBar, &ToolBar::gridToggled, m_canvasView, &CanvasView::setGridVisible);
    connect(m_canvasView, &CanvasView::zoomChanged, m_toolBar, &ToolBar::setZoomLevel);
    
    setCentralWidget(centralWidget);
    
    // Default size
    resize(1200, 800);
    
    // Center on screen
    if (QScreen *screen = QApplication::primaryScreen()) {
        QRect screenGeometry = screen->availableGeometry();
        move((screenGeometry.width() - width()) / 2,
             (screenGeometry.height() - height()) / 2);
    }
}

void MainWindow::setupMenus()
{
    // Context menu is shown on right-click via contextMenuEvent
}

void MainWindow::setupShortcuts()
{
    // File operations
    QShortcut *newShortcut = new QShortcut(QKeySequence::New, this);
    connect(newShortcut, &QShortcut::activated, this, &MainWindow::newBoard);
    
    QShortcut *openShortcut = new QShortcut(QKeySequence::Open, this);
    connect(openShortcut, &QShortcut::activated, this, &MainWindow::openBoard);
    
    QShortcut *saveShortcut = new QShortcut(QKeySequence::Save, this);
    connect(saveShortcut, &QShortcut::activated, this, &MainWindow::saveCurrentBoard);
    
    QShortcut *saveAsShortcut = new QShortcut(QKeySequence::SaveAs, this);
    connect(saveAsShortcut, &QShortcut::activated, this, &MainWindow::saveBoardAs);
    
    // Edit operations
    QShortcut *pasteShortcut = new QShortcut(QKeySequence::Paste, this);
    connect(pasteShortcut, &QShortcut::activated, this, [this]() {
        m_canvasScene->pasteFromClipboard();
    });
    
    QShortcut *deleteShortcut = new QShortcut(QKeySequence::Delete, this);
    connect(deleteShortcut, &QShortcut::activated, this, [this]() {
        m_canvasScene->deleteSelected();
    });
    
    QShortcut *selectAllShortcut = new QShortcut(QKeySequence::SelectAll, this);
    connect(selectAllShortcut, &QShortcut::activated, this, [this]() {
        m_canvasScene->selectAll();
    });
    
    QShortcut *undoShortcut = new QShortcut(QKeySequence::Undo, this);
    connect(undoShortcut, &QShortcut::activated, this, [this]() {
        m_canvasScene->undo();
    });
    
    QShortcut *redoShortcut = new QShortcut(QKeySequence::Redo, this);
    connect(redoShortcut, &QShortcut::activated, this, [this]() {
        m_canvasScene->redo();
    });
    
    // View operations
    QShortcut *fitShortcut = new QShortcut(QKeySequence(Qt::Key_F), this);
    connect(fitShortcut, &QShortcut::activated, this, [this]() {
        m_canvasView->fitAll();
    });
    
    QShortcut *resetShortcut = new QShortcut(QKeySequence(Qt::Key_R), this);
    connect(resetShortcut, &QShortcut::activated, this, [this]() {
        m_canvasView->resetView();
    });
    
    QShortcut *topShortcut = new QShortcut(QKeySequence(Qt::Key_T), this);
    connect(topShortcut, &QShortcut::activated, this, &MainWindow::toggleAlwaysOnTop);
    
    // Zoom
    QShortcut *zoomInShortcut = new QShortcut(QKeySequence::ZoomIn, this);
    connect(zoomInShortcut, &QShortcut::activated, this, [this]() {
        m_canvasView->zoomIn();
    });
    
    QShortcut *zoomOutShortcut = new QShortcut(QKeySequence::ZoomOut, this);
    connect(zoomOutShortcut, &QShortcut::activated, this, [this]() {
        m_canvasView->zoomOut();
    });
    
    QShortcut *resetZoomShortcut = new QShortcut(QKeySequence(Qt::Key_0), this);
    connect(resetZoomShortcut, &QShortcut::activated, this, [this]() {
        m_canvasView->resetZoom();
    });
    
    // Escape to deselect
    QShortcut *escapeShortcut = new QShortcut(QKeySequence(Qt::Key_Escape), this);
    connect(escapeShortcut, &QShortcut::activated, this, [this]() {
        m_canvasScene->clearSelection();
    });
}

void MainWindow::setFramelessWindow()
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::Window);
    setAttribute(Qt::WA_TranslucentBackground, m_isTransparent);
    
    if (m_alwaysOnTop) {
        setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
    }
}

void MainWindow::loadSettings()
{
    QSettings settings;
    
    m_alwaysOnTop = settings.value("window/alwaysOnTop", false).toBool();
    m_isTransparent = settings.value("window/transparent", false).toBool();
    m_opacity = settings.value("window/opacity", 100).toInt();
    
    QByteArray geometry = settings.value("window/geometry").toByteArray();
    if (!geometry.isEmpty()) {
        restoreGeometry(geometry);
    }
    
    QString lastFile = settings.value("file/lastOpened").toString();
    if (!lastFile.isEmpty() && QFile::exists(lastFile)) {
        // Could auto-load last file here
    }
    
    setWindowOpacity(m_opacity / 100.0);
}

void MainWindow::saveSettings()
{
    QSettings settings;
    
    settings.setValue("window/alwaysOnTop", m_alwaysOnTop);
    settings.setValue("window/transparent", m_isTransparent);
    settings.setValue("window/opacity", m_opacity);
    settings.setValue("window/geometry", saveGeometry());
    
    if (!m_currentFilePath.isEmpty()) {
        settings.setValue("file/lastOpened", m_currentFilePath);
    }
}

void MainWindow::connectToServer(const QString &url, const QString &roomId)
{
    QString room = roomId;
    if (room.isEmpty()) {
        room = QUuid::createUuid().toString(QUuid::WithoutBraces).left(8);
    }
    m_collabManager->connectToServer(url, room);
}

void MainWindow::loadBoard(const QString &filePath)
{
    if (!QFile::exists(filePath)) {
        QMessageBox::warning(this, "Error", "File not found: " + filePath);
        return;
    }
    
    Board *newBoard = BoardSerializer::load(filePath);
    if (newBoard) {
        delete m_board;
        m_board = newBoard;
        m_board->setParent(this);
        m_canvasScene->setBoard(m_board);
        m_collabManager->setBoard(m_board);
        m_currentFilePath = filePath;
        m_isModified = false;
        updateWindowTitle();
    } else {
        QMessageBox::warning(this, "Error", "Failed to load board file.");
    }
}

void MainWindow::saveBoard(const QString &filePath)
{
    if (BoardSerializer::save(m_board, filePath)) {
        m_currentFilePath = filePath;
        m_isModified = false;
        updateWindowTitle();
    } else {
        QMessageBox::warning(this, "Error", "Failed to save board file.");
    }
}

void MainWindow::newBoard()
{
    if (m_isModified) {
        int result = QMessageBox::question(this, "Unsaved Changes",
            "Do you want to save changes before creating a new board?",
            QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
        
        if (result == QMessageBox::Save) {
            saveCurrentBoard();
        } else if (result == QMessageBox::Cancel) {
            return;
        }
    }
    
    delete m_board;
    m_board = new Board(this);
    m_canvasScene->setBoard(m_board);
    m_collabManager->setBoard(m_board);
    m_currentFilePath.clear();
    m_isModified = false;
    m_canvasView->resetView();
    updateWindowTitle();
}

void MainWindow::openBoard()
{
    QString filePath = QFileDialog::getOpenFileName(this, "Open Board",
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
        "CollabRef Boards (*.cref);;All Files (*)");
    
    if (!filePath.isEmpty()) {
        loadBoard(filePath);
    }
}

void MainWindow::saveBoardAs()
{
    QString filePath = QFileDialog::getSaveFileName(this, "Save Board As",
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/untitled.cref",
        "CollabRef Boards (*.cref);;All Files (*)");
    
    if (!filePath.isEmpty()) {
        if (!filePath.endsWith(".cref", Qt::CaseInsensitive)) {
            filePath += ".cref";
        }
        saveBoard(filePath);
    }
}

void MainWindow::saveCurrentBoard()
{
    if (m_currentFilePath.isEmpty()) {
        saveBoardAs();
    } else {
        saveBoard(m_currentFilePath);
    }
}

void MainWindow::hostSession()
{
    if (m_isHosting) {
        // Already hosting, show info
        QString info = QString("Already hosting on port %1").arg(m_server->port());
        if (m_ngrokUrl.isEmpty()) {
            info += QString("\n\nLocal IP: %1").arg(m_server->localAddress());
        } else {
            info += QString("\n\nngrok URL: %1").arg(m_ngrokUrl);
            QApplication::clipboard()->setText(m_ngrokUrl);
            info += "\n\n(Copied to clipboard!)";
        }
        QMessageBox::information(this, "Already Hosting", info);
        return;
    }
    
    // Ask how to host
    QMessageBox msgBox(this);
    msgBox.setWindowTitle("Host Session");
    msgBox.setText("How do you want to host?");
    msgBox.setInformativeText(
        "Same Network - Friends on same WiFi\n"
        "Using ngrok - Friends anywhere (easiest!)\n"
        "Over Internet - Friends anywhere (needs port forwarding)");
    QAbstractButton *localBtn = msgBox.addButton("Same Network", QMessageBox::AcceptRole);
    QAbstractButton *ngrokBtn = msgBox.addButton("Using ngrok", QMessageBox::ActionRole);
    QAbstractButton *internetBtn = msgBox.addButton("Over Internet", QMessageBox::ActionRole);
    msgBox.addButton(QMessageBox::Cancel);
    msgBox.exec();
    
    QAbstractButton *clicked = msgBox.clickedButton();
    if (clicked == nullptr || clicked == msgBox.button(QMessageBox::Cancel)) {
        return;
    }
    
    bool isNgrok = (clicked == ngrokBtn);
    bool isInternet = (clicked == internetBtn);
    bool isLocal = (clicked == localBtn);
    Q_UNUSED(isLocal)
    
    // Start the built-in server
    quint16 port = 8080;
    if (!m_server->start(port)) {
        port = 8081;
        if (!m_server->start(port)) {
            QMessageBox::warning(this, "Error", 
                "Could not start server. Port may be in use.\n\n"
                "Try closing other apps that might use port 8080.");
            return;
        }
    }
    
    m_isHosting = true;
    
    // Connect to our own server
    QString localUrl = QString("ws://127.0.0.1:%1").arg(port);
    connectToServer(localUrl, "host");
    
    QString localIP = m_server->localAddress();
    
    if (isNgrok) {
        // Launch ngrok automatically
        m_titleBar->showNotification("Starting ngrok...");
        
        // Kill any existing ngrok first
        QProcess::execute("taskkill", QStringList() << "/F" << "/IM" << "ngrok.exe");
        
        // Start ngrok
        QProcess *ngrokProcess = new QProcess(this);
        ngrokProcess->setProgram("ngrok");
        ngrokProcess->setArguments(QStringList() << "http" << QString::number(port));
        ngrokProcess->start();
        
        if (!ngrokProcess->waitForStarted(5000)) {
            QMessageBox::warning(this, "ngrok Error",
                "Could not start ngrok.\n\n"
                "Make sure ngrok is installed and in your PATH.\n\n"
                "Download from: https://ngrok.com/download\n"
                "Then run: ngrok config add-authtoken YOUR_TOKEN");
            return;
        }
        
        // Wait a moment for ngrok to initialize
        QThread::msleep(2000);
        
        // Query ngrok API to get the public URL
        QNetworkAccessManager *manager = new QNetworkAccessManager(this);
        QNetworkReply *reply = manager->get(QNetworkRequest(QUrl("http://127.0.0.1:4040/api/tunnels")));
        
        QEventLoop loop;
        QTimer timer;
        timer.setSingleShot(true);
        connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
        timer.start(5000);
        loop.exec();
        
        QString ngrokUrl;
        if (reply->isFinished() && reply->error() == QNetworkReply::NoError) {
            QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
            QJsonArray tunnels = doc.object()["tunnels"].toArray();
            for (const QJsonValue &tunnel : tunnels) {
                QString publicUrl = tunnel.toObject()["public_url"].toString();
                if (publicUrl.startsWith("https://")) {
                    // Convert https:// to wss:// for WebSocket
                    ngrokUrl = "wss://" + publicUrl.mid(8);
                    break;
                }
            }
        }
        reply->deleteLater();
        manager->deleteLater();
        
        if (ngrokUrl.isEmpty()) {
            QMessageBox::warning(this, "ngrok Error",
                "Could not get ngrok URL.\n\n"
                "Make sure you've set up ngrok with your auth token:\n"
                "ngrok config add-authtoken YOUR_TOKEN\n\n"
                "You can still host - run 'ngrok http 8080' manually\n"
                "and share the URL with friends.");
        } else {
            m_ngrokUrl = ngrokUrl;
            QApplication::clipboard()->setText(ngrokUrl);
            
            QMessageBox::information(this, "Ready to Collaborate!",
                QString("🎉 You're hosting!\n\n"
                        "Share this URL with friends:\n\n"
                        "%1\n\n"
                        "✓ Copied to clipboard!\n\n"
                        "They just paste it in 'Join Session'")
                .arg(ngrokUrl));
        }
    } else if (isInternet) {
        // Try to get public IP
        QString publicIP = "Could not detect";
        
        QNetworkAccessManager *manager = new QNetworkAccessManager(this);
        QNetworkReply *reply = manager->get(QNetworkRequest(QUrl("https://api.ipify.org")));
        
        QEventLoop loop;
        QTimer timer;
        timer.setSingleShot(true);
        connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
        timer.start(3000);
        loop.exec();
        
        if (reply->isFinished() && reply->error() == QNetworkReply::NoError) {
            publicIP = QString::fromUtf8(reply->readAll()).trimmed();
        }
        reply->deleteLater();
        manager->deleteLater();
        
        QString instructions = QString(
            "=== INTERNET HOSTING ===\n\n"
            "Your Public IP: %1\n"
            "Port: %2\n\n"
            "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n"
            "ONE-TIME SETUP (Port Forwarding):\n"
            "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n"
            "1. Open router settings (192.168.1.1)\n"
            "2. Find 'Port Forwarding'\n"
            "3. Add rule: Port %2 → %3:%2 (TCP)\n"
            "4. Save\n\n"
            "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n"
            "SHARE WITH FRIEND:\n"
            "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n"
            "IP: %1\n"
            "Port: %2"
        ).arg(publicIP).arg(port).arg(localIP);
        
        QMessageBox infoBox(this);
        infoBox.setWindowTitle("Internet Hosting Setup");
        infoBox.setText(instructions);
        infoBox.setTextInteractionFlags(Qt::TextSelectableByMouse);
        
        QAbstractButton *copyBtn = infoBox.addButton("Copy IP:Port", QMessageBox::ActionRole);
        infoBox.addButton(QMessageBox::Ok);
        infoBox.exec();
        
        if (infoBox.clickedButton() == copyBtn) {
            QApplication::clipboard()->setText(QString("%1:%2").arg(publicIP).arg(port));
        }
    } else {
        // Local network
        QString info = QString("%1:%2").arg(localIP).arg(port);
        QApplication::clipboard()->setText(info);
        
        QMessageBox::information(this, "Session Started",
            QString("You're hosting on your local network!\n\n"
                    "Share with your friend:\n\n"
                    "%1\n\n"
                    "✓ Copied to clipboard!\n\n"
                    "They click 'Join Session' and paste it.")
            .arg(info));
    }
}

void MainWindow::joinSession()
{
    bool ok;
    QString address = QInputDialog::getText(this, "Join Session",
        "Enter host's address:\n\n"
        "Examples:\n"
        "  192.168.1.100 (same network)\n"
        "  85.123.45.67:8080 (internet)\n"
        "  wss://abc123.ngrok-free.app (ngrok)",
        QLineEdit::Normal, "", &ok);
    
    if (!ok || address.isEmpty()) return;
    
    // Handle ngrok URLs (already complete)
    if (address.contains("ngrok") || address.contains(".app") || address.contains(".io")) {
        if (!address.startsWith("ws://") && !address.startsWith("wss://")) {
            address = "wss://" + address;
        }
        connectToServer(address, "guest");
        return;
    }
    
    // Handle IP addresses
    QString port = "8080";
    if (!address.contains(":")) {
        port = QInputDialog::getText(this, "Join Session",
            "Enter port number:\n\n(Usually 8080)",
            QLineEdit::Normal, "8080", &ok);
        if (!ok) return;
        address += ":" + port;
    }
    
    // Add ws:// for regular IPs
    if (!address.startsWith("ws://") && !address.startsWith("wss://")) {
        address = "ws://" + address;
    }
    
    connectToServer(address, "guest");
}

void MainWindow::leaveSession()
{
    m_collabManager->disconnect();
    
    if (m_isHosting) {
        m_server->stop();
        m_isHosting = false;
        m_ngrokUrl.clear();
        
        // Kill ngrok process if running
        QProcess::startDetached("taskkill", QStringList() << "/F" << "/IM" << "ngrok.exe");
    }
    
    // Note: We intentionally keep all local content so users can continue working
    // and sync when they reconnect
    m_titleBar->showNotification("Disconnected - your work is preserved");
}

void MainWindow::toggleAlwaysOnTop()
{
    m_alwaysOnTop = !m_alwaysOnTop;
    
    Qt::WindowFlags flags = windowFlags();
    if (m_alwaysOnTop) {
        flags |= Qt::WindowStaysOnTopHint;
    } else {
        flags &= ~Qt::WindowStaysOnTopHint;
    }
    
    setWindowFlags(flags);
    show();
}

void MainWindow::toggleTransparency()
{
    m_isTransparent = !m_isTransparent;
    setAttribute(Qt::WA_TranslucentBackground, m_isTransparent);
}

void MainWindow::setOpacity(int percent)
{
    m_opacity = qBound(10, percent, 100);
    setWindowOpacity(m_opacity / 100.0);
}

void MainWindow::showAbout()
{
    QMessageBox::about(this, "About CollabRef",
        "<h2>CollabRef 1.0</h2>"
        "<p>A collaborative reference board application.</p>"
        "<p>Built with Qt and WebSockets for real-time collaboration.</p>");
}

void MainWindow::showSettings()
{
    // TODO: Implement settings dialog
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (m_isModified) {
        int result = QMessageBox::question(this, "Unsaved Changes",
            "Do you want to save changes before closing?",
            QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
        
        if (result == QMessageBox::Save) {
            saveCurrentBoard();
        } else if (result == QMessageBox::Cancel) {
            event->ignore();
            return;
        }
    }
    
    if (m_collabManager->isConnected()) {
        m_collabManager->disconnect();
    }
    
    if (m_isHosting && m_server) {
        m_server->stop();
        m_isHosting = false;
    }
    
    saveSettings();
    event->accept();
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    // Let the canvas view handle key events through Qt's event system
    QMainWindow::keyPressEvent(event);
}

void MainWindow::keyReleaseEvent(QKeyEvent *event)
{
    QMainWindow::keyReleaseEvent(event);
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls() || event->mimeData()->hasImage()) {
        event->acceptProposedAction();
    }
}

void MainWindow::dropEvent(QDropEvent *event)
{
    const QMimeData *mimeData = event->mimeData();
    QPointF scenePos = m_canvasView->mapToScene(
        m_canvasView->mapFromGlobal(mapToGlobal(event->pos())));
    
    if (mimeData->hasUrls()) {
        for (const QUrl &url : mimeData->urls()) {
            if (url.isLocalFile()) {
                QString filePath = url.toLocalFile();
                QImage image(filePath);
                if (!image.isNull()) {
                    m_canvasScene->addImageItem(image, scenePos, filePath);
                }
            }
        }
    } else if (mimeData->hasImage()) {
        QImage image = qvariant_cast<QImage>(mimeData->imageData());
        if (!image.isNull()) {
            m_canvasScene->addImageItem(image, scenePos);
        }
    }
    
    m_isModified = true;
    event->acceptProposedAction();
}

void MainWindow::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        // Check for resize
        m_resizeEdge = None;
        QPoint pos = event->pos();
        QRect rect = this->rect();
        
        if (pos.x() <= RESIZE_MARGIN) m_resizeEdge = (ResizeEdge)(m_resizeEdge | Left);
        if (pos.x() >= rect.width() - RESIZE_MARGIN) m_resizeEdge = (ResizeEdge)(m_resizeEdge | Right);
        if (pos.y() <= RESIZE_MARGIN) m_resizeEdge = (ResizeEdge)(m_resizeEdge | Top);
        if (pos.y() >= rect.height() - RESIZE_MARGIN) m_resizeEdge = (ResizeEdge)(m_resizeEdge | Bottom);
        
        if (m_resizeEdge != None) {
            m_resizeStartPos = event->globalPos();
            m_resizeStartGeometry = geometry();
            return;
        }
        
        // Window dragging from title bar handled by TitleBar
    }
    
    QMainWindow::mousePressEvent(event);
}

void MainWindow::mouseMoveEvent(QMouseEvent *event)
{
    if (m_resizeEdge != None && event->buttons() & Qt::LeftButton) {
        QPoint delta = event->globalPos() - m_resizeStartPos;
        QRect newGeom = m_resizeStartGeometry;
        
        if (m_resizeEdge & Left) {
            newGeom.setLeft(newGeom.left() + delta.x());
        }
        if (m_resizeEdge & Right) {
            newGeom.setRight(newGeom.right() + delta.x());
        }
        if (m_resizeEdge & Top) {
            newGeom.setTop(newGeom.top() + delta.y());
        }
        if (m_resizeEdge & Bottom) {
            newGeom.setBottom(newGeom.bottom() + delta.y());
        }
        
        if (newGeom.width() >= minimumWidth() && newGeom.height() >= minimumHeight()) {
            setGeometry(newGeom);
        }
        return;
    }
    
    // Update cursor for resize handles
    QPoint pos = event->pos();
    QRect rect = this->rect();
    
    bool onLeft = pos.x() <= RESIZE_MARGIN;
    bool onRight = pos.x() >= rect.width() - RESIZE_MARGIN;
    bool onTop = pos.y() <= RESIZE_MARGIN;
    bool onBottom = pos.y() >= rect.height() - RESIZE_MARGIN;
    
    if ((onLeft && onTop) || (onRight && onBottom)) {
        setCursor(Qt::SizeFDiagCursor);
    } else if ((onRight && onTop) || (onLeft && onBottom)) {
        setCursor(Qt::SizeBDiagCursor);
    } else if (onLeft || onRight) {
        setCursor(Qt::SizeHorCursor);
    } else if (onTop || onBottom) {
        setCursor(Qt::SizeVerCursor);
    } else {
        setCursor(Qt::ArrowCursor);
    }
    
    QMainWindow::mouseMoveEvent(event);
}

void MainWindow::mouseReleaseEvent(QMouseEvent *event)
{
    m_resizeEdge = None;
    m_isDragging = false;
    QMainWindow::mouseReleaseEvent(event);
}

void MainWindow::mouseDoubleClickEvent(QMouseEvent *event)
{
    QMainWindow::mouseDoubleClickEvent(event);
}

void MainWindow::wheelEvent(QWheelEvent *event)
{
    QMainWindow::wheelEvent(event);
}

void MainWindow::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu menu(this);
    
    // File menu
    QMenu *fileMenu = menu.addMenu("File");
    
    QAction *newAction = fileMenu->addAction("New");
    connect(newAction, &QAction::triggered, this, &MainWindow::newBoard);
    
    QAction *openAction = fileMenu->addAction("Open...");
    connect(openAction, &QAction::triggered, this, &MainWindow::openBoard);
    
    QAction *saveAction = fileMenu->addAction("Save");
    connect(saveAction, &QAction::triggered, this, &MainWindow::saveCurrentBoard);
    
    QAction *saveAsAction = fileMenu->addAction("Save As...");
    connect(saveAsAction, &QAction::triggered, this, &MainWindow::saveBoardAs);
    
    fileMenu->addSeparator();
    
    QAction *addImageAction = fileMenu->addAction("Add Image...");
    connect(addImageAction, &QAction::triggered, this, [this]() {
        QStringList files = QFileDialog::getOpenFileNames(this, "Add Images",
            QStandardPaths::writableLocation(QStandardPaths::PicturesLocation),
            "Images (*.png *.jpg *.jpeg *.bmp *.gif *.webp *.ico);;All Files (*)");
        
        if (files.isEmpty()) {
            return;
        }
        
        QPointF pos(100, 100);
        for (const QString &file : files) {
            QFileInfo fileInfo(file);
            if (!fileInfo.exists()) {
                continue;
            }
            
            // For GIFs, pass the file path to enable animation
            if (fileInfo.suffix().toLower() == "gif") {
                QImage firstFrame(file);
                if (!firstFrame.isNull()) {
                    m_canvasScene->addImageItem(firstFrame, pos, file);
                    pos += QPointF(50, 50);
                }
            } else {
                QImageReader reader(file);
                reader.setAutoTransform(true);
                QImage image = reader.read();
                
                if (!image.isNull()) {
                    m_canvasScene->addImageItem(image, pos, file);
                    pos += QPointF(50, 50);
                }
            }
        }
        
        m_canvasView->viewport()->update();
        m_canvasScene->update();
    });
    
    QAction *addTextAction = fileMenu->addAction("Add Text");
    connect(addTextAction, &QAction::triggered, this, [this]() {
        bool ok;
        QString text = QInputDialog::getText(this, "Add Text",
            "Enter text:", QLineEdit::Normal, "Your text here", &ok);
        
        if (ok && !text.isEmpty()) {
            QPointF center = m_canvasView->mapToScene(
                m_canvasView->viewport()->rect().center());
            m_canvasScene->addTextItem(text, center);
        }
    });
    
    // Edit menu
    QMenu *editMenu = menu.addMenu("Edit");
    
    QAction *undoAction = editMenu->addAction("Undo");
    connect(undoAction, &QAction::triggered, m_canvasScene, &CanvasScene::undo);
    
    QAction *redoAction = editMenu->addAction("Redo");
    connect(redoAction, &QAction::triggered, m_canvasScene, &CanvasScene::redo);
    
    editMenu->addSeparator();
    
    QAction *pasteAction = editMenu->addAction("Paste");
    connect(pasteAction, &QAction::triggered, m_canvasScene, &CanvasScene::pasteFromClipboard);
    
    QAction *deleteAction = editMenu->addAction("Delete");
    connect(deleteAction, &QAction::triggered, m_canvasScene, &CanvasScene::deleteSelected);
    
    QAction *selectAllAction = editMenu->addAction("Select All");
    connect(selectAllAction, &QAction::triggered, m_canvasScene, &CanvasScene::selectAll);
    
    // View menu
    QMenu *viewMenu = menu.addMenu("View");
    
    QAction *fitAllAction = viewMenu->addAction("Fit All");
    connect(fitAllAction, &QAction::triggered, m_canvasView, &CanvasView::fitAll);
    
    QAction *resetViewAction = viewMenu->addAction("Reset View");
    connect(resetViewAction, &QAction::triggered, m_canvasView, &CanvasView::resetView);
    
    QAction *resetZoomAction = viewMenu->addAction("Reset Zoom");
    connect(resetZoomAction, &QAction::triggered, m_canvasView, &CanvasView::resetZoom);
    
    viewMenu->addSeparator();
    
    QAction *scaleWithWindowAction = viewMenu->addAction("Scale With Window");
    scaleWithWindowAction->setCheckable(true);
    scaleWithWindowAction->setChecked(m_canvasView->isScaleWithWindow());
    connect(scaleWithWindowAction, &QAction::triggered, m_canvasView, &CanvasView::setScaleWithWindow);
    
    viewMenu->addSeparator();
    
    QAction *alwaysOnTopAction = viewMenu->addAction("Always on Top");
    alwaysOnTopAction->setCheckable(true);
    alwaysOnTopAction->setChecked(m_alwaysOnTop);
    connect(alwaysOnTopAction, &QAction::triggered, this, &MainWindow::toggleAlwaysOnTop);
    
    // Opacity submenu
    QMenu *opacityMenu = viewMenu->addMenu("Opacity");
    for (int i = 100; i >= 20; i -= 20) {
        QAction *action = opacityMenu->addAction(QString("%1%").arg(i));
        action->setCheckable(true);
        action->setChecked(m_opacity >= i - 10 && m_opacity <= i + 10);
        connect(action, &QAction::triggered, this, [this, i]() {
            setOpacity(i);
        });
    }
    
    // Collaboration menu
    QMenu *collabMenu = menu.addMenu("Collaborate");
    if (m_isHosting) {
        // Show hosting info
        QAction *infoAction = collabMenu->addAction(
            QString("Hosting on %1:%2").arg(m_server->localAddress()).arg(m_server->port()));
        infoAction->setEnabled(false);
        
        QAction *clientsAction = collabMenu->addAction(
            QString("%1 connected").arg(m_server->clientCount()));
        clientsAction->setEnabled(false);
        
        collabMenu->addSeparator();
        
        QAction *copyAction = collabMenu->addAction("Copy Connection Info");
        connect(copyAction, &QAction::triggered, this, [this]() {
            QString info = QString("%1:%2").arg(m_server->localAddress()).arg(m_server->port());
            QApplication::clipboard()->setText(info);
            m_titleBar->showNotification("Copied: " + info);
        });
        
        QAction *syncAction = collabMenu->addAction("Sync Now");
        connect(syncAction, &QAction::triggered, this, [this]() {
            m_collabManager->pushLocalState();
            m_titleBar->showNotification("Syncing...");
        });
        
        collabMenu->addSeparator();
        
        QAction *stopAction = collabMenu->addAction("Stop Hosting");
        connect(stopAction, &QAction::triggered, this, &MainWindow::leaveSession);
    } else if (m_collabManager->isConnected()) {
        QAction *leaveAction = collabMenu->addAction("Leave Session");
        connect(leaveAction, &QAction::triggered, this, &MainWindow::leaveSession);
        
        collabMenu->addSeparator();
        
        QAction *syncAction = collabMenu->addAction("Sync Now");
        connect(syncAction, &QAction::triggered, this, [this]() {
            m_collabManager->pushLocalState();
            m_titleBar->showNotification("Syncing...");
        });
        
        collabMenu->addSeparator();
        QAction *statusAction = collabMenu->addAction("Connected");
        statusAction->setEnabled(false);
    } else {
        QAction *hostAction = collabMenu->addAction("Host Session");
        connect(hostAction, &QAction::triggered, this, &MainWindow::hostSession);
        
        QAction *joinAction = collabMenu->addAction("Join Session...");
        connect(joinAction, &QAction::triggered, this, &MainWindow::joinSession);
    }
    
    menu.addSeparator();
    
    QAction *aboutAction = menu.addAction("About");
    connect(aboutAction, &QAction::triggered, this, &MainWindow::showAbout);
    
    menu.addSeparator();
    
    QAction *exitAction = menu.addAction("Exit");
    connect(exitAction, &QAction::triggered, this, &QMainWindow::close);
    
    menu.exec(event->globalPos());
}

void MainWindow::onConnectionStatusChanged(bool connected)
{
    updateWindowTitle();
    updateConnectionIndicator();
    
    if (connected) {
        m_autoSaveTimer->start();
        m_titleBar->showNotification("Connected!");
    } else {
        m_autoSaveTimer->stop();
        // Only show disconnect notification if we were actually in a session
        // (not when initially loading)
        if (m_collabManager->userCount() > 0 || m_isHosting) {
            m_titleBar->showNotification("Connection lost - work preserved locally");
        }
    }
}

void MainWindow::onUserJoined(const QString &userId, const QString &userName)
{
    Q_UNUSED(userId)
    m_titleBar->showNotification(QString("%1 joined").arg(userName));
}

void MainWindow::onUserLeft(const QString &userId)
{
    Q_UNUSED(userId)
    m_titleBar->showNotification("A collaborator left");
}

void MainWindow::onBoardSynced()
{
    m_canvasView->update();
}

void MainWindow::updateWindowTitle()
{
    QString title = "CollabRef";
    
    if (!m_currentFilePath.isEmpty()) {
        QFileInfo fileInfo(m_currentFilePath);
        title += " - " + fileInfo.fileName();
    } else {
        title += " - Untitled";
    }
    
    if (m_isModified) {
        title += "*";
    }
    
    if (m_collabManager && m_collabManager->isConnected()) {
        title += QString(" [%1]").arg(m_collabManager->roomId());
    }
    
    setWindowTitle(title);
    if (m_titleBar) {
        m_titleBar->setTitle(title);
    }
}

void MainWindow::updateConnectionIndicator()
{
    if (m_titleBar) {
        m_titleBar->setConnectionStatus(m_collabManager->isConnected());
    }
}

void MainWindow::autoSave()
{
    if (m_isModified && !m_currentFilePath.isEmpty()) {
        saveBoard(m_currentFilePath);
    }
}

void MainWindow::onServerClientConnected(const QString &clientId)
{
    Q_UNUSED(clientId)
    if (m_isHosting) {
        m_titleBar->showNotification(QString("Someone joined! (%1 connected)")
            .arg(m_server->clientCount()));
    }
}

void MainWindow::onServerClientDisconnected(const QString &clientId)
{
    Q_UNUSED(clientId)
    if (m_isHosting) {
        m_titleBar->showNotification(QString("Someone left (%1 connected)")
            .arg(m_server->clientCount()));
    }
}

QString MainWindow::loadServerConfig()
{
    // Look for server.conf in the same directory as the executable
    QString configPath = QCoreApplication::applicationDirPath() + "/server.conf";
    
    // Also check parent directory (for development)
    if (!QFile::exists(configPath)) {
        configPath = QCoreApplication::applicationDirPath() + "/../server.conf";
    }
    
    // Also check home directory
    if (!QFile::exists(configPath)) {
        configPath = QDir::homePath() + "/.collabref/server.conf";
    }
    
    if (QFile::exists(configPath)) {
        QFile file(configPath);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&file);
            while (!in.atEnd()) {
                QString line = in.readLine().trimmed();
                if (line.isEmpty() || line.startsWith('#')) continue;
                
                if (line.startsWith("server=")) {
                    m_configuredServerUrl = line.mid(7).trimmed();
                } else if (line.startsWith("room=")) {
                    m_configuredRoomId = line.mid(5).trimmed();
                }
            }
            file.close();
        }
    }
    
    return m_configuredServerUrl;
}

void MainWindow::autoConnectToServer()
{
    QString serverUrl = loadServerConfig();
    
    // Default to localhost if no config
    if (serverUrl.isEmpty()) {
        serverUrl = "ws://localhost:8080";
        m_configuredServerUrl = serverUrl;
    }
    
    if (m_configuredRoomId.isEmpty()) {
        m_configuredRoomId = "main";
    }
    
    // Try to connect first - if it fails, we'll become the host
    m_collabManager->connectToServer(serverUrl, m_configuredRoomId);
    
    // Setup reconnect timer that also tries to host if connection fails
    if (!m_reconnectTimer) {
        m_reconnectTimer = new QTimer(this);
        m_reconnectTimer->setInterval(2000);
        connect(m_reconnectTimer, &QTimer::timeout, this, &MainWindow::tryReconnect);
    }
    m_reconnectTimer->start();
}

void MainWindow::tryReconnect()
{
    if (m_collabManager->isConnected()) {
        // Already connected, stop trying
        if (!m_isHosting) {
            m_titleBar->showNotification("Connected!");
        }
        m_reconnectTimer->setInterval(5000);  // Slow down checks
        return;
    }
    
    if (m_isHosting) {
        // We're the host, just keep running
        return;
    }
    
    // Not connected and not hosting - try to become host
    static int attempts = 0;
    attempts++;
    
    if (attempts >= 2) {
        // Failed to connect twice, become the host
        m_titleBar->showNotification("Starting server...");
        
        // Save board next to the executable (shared location for all users)
        QString sharedPath = QCoreApplication::applicationDirPath() + "/shared_board.json";
        m_server->setSaveFile(sharedPath);
        
        if (m_server->start(8080)) {
            m_isHosting = true;
            
            // Connect to our own server
            QTimer::singleShot(500, this, [this]() {
                m_collabManager->connectToServer("ws://localhost:8080", m_configuredRoomId);
                m_titleBar->showNotification("Hosting - others can join!");
            });
            
            attempts = 0;
        } else {
            // Port in use, keep trying to connect
            m_collabManager->connectToServer(m_configuredServerUrl, m_configuredRoomId);
        }
    } else {
        // Try connecting again
        m_collabManager->connectToServer(m_configuredServerUrl, m_configuredRoomId);
    }
}
