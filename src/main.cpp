/**
 * CollabRef - Collaborative Reference Board
 * A PureRef-like application with real-time collaboration
 */

#include <QApplication>
#include <QCommandLineParser>
#include <QStyleFactory>
#include <QDir>
#include "MainWindow.h"

int main(int argc, char *argv[])
{
    // Enable high DPI scaling
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
#endif

    QApplication app(argc, argv);
    
    // Set plugin path explicitly BEFORE anything else
    QString appDir = QCoreApplication::applicationDirPath();
    
    // Qt looks for plugins in these locations
    app.addLibraryPath(appDir);
    app.addLibraryPath(appDir + "/plugins");
    
    // Also try to load from vcpkg installed location (for development)
    app.addLibraryPath(appDir + "/../../vcpkg/installed/x64-windows/Qt6/plugins");
    
    app.setApplicationName("CollabRef");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("CollabRef");
    app.setOrganizationDomain("collabref.app");

    // Set dark fusion style
    app.setStyle(QStyleFactory::create("Fusion"));
    
    QPalette darkPalette;
    darkPalette.setColor(QPalette::Window, QColor(45, 45, 48));
    darkPalette.setColor(QPalette::WindowText, Qt::white);
    darkPalette.setColor(QPalette::Base, QColor(30, 30, 30));
    darkPalette.setColor(QPalette::AlternateBase, QColor(45, 45, 48));
    darkPalette.setColor(QPalette::ToolTipBase, QColor(45, 45, 48));
    darkPalette.setColor(QPalette::ToolTipText, Qt::white);
    darkPalette.setColor(QPalette::Text, Qt::white);
    darkPalette.setColor(QPalette::Button, QColor(45, 45, 48));
    darkPalette.setColor(QPalette::ButtonText, Qt::white);
    darkPalette.setColor(QPalette::BrightText, Qt::red);
    darkPalette.setColor(QPalette::Link, QColor(42, 130, 218));
    darkPalette.setColor(QPalette::Highlight, QColor(42, 130, 218));
    darkPalette.setColor(QPalette::HighlightedText, Qt::black);
    app.setPalette(darkPalette);

    // Command line options
    QCommandLineParser parser;
    parser.setApplicationDescription("Collaborative Reference Board");
    parser.addHelpOption();
    parser.addVersionOption();
    
    QCommandLineOption serverOption(QStringList() << "s" << "server",
        "Connect to collaboration server", "url");
    parser.addOption(serverOption);
    
    QCommandLineOption roomOption(QStringList() << "r" << "room",
        "Join specific room", "room-id");
    parser.addOption(roomOption);
    
    QCommandLineOption fileOption(QStringList() << "f" << "file",
        "Open board file", "path");
    parser.addOption(fileOption);
    
    parser.process(app);

    // Create main window
    MainWindow window;
    
    // Handle command line arguments
    if (parser.isSet(serverOption)) {
        QString serverUrl = parser.value(serverOption);
        QString roomId = parser.isSet(roomOption) ? parser.value(roomOption) : "";
        window.connectToServer(serverUrl, roomId);
    }
    
    if (parser.isSet(fileOption)) {
        window.loadBoard(parser.value(fileOption));
    }
    
    window.show();

    return app.exec();
}
