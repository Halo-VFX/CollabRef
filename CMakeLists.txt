cmake_minimum_required(VERSION 3.16)
project(CollabRef VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Find Qt6 (fallback to Qt5)
find_package(Qt6 COMPONENTS Core Gui Widgets Network WebSockets OpenGLWidgets QUIET)
if(NOT Qt6_FOUND)
    find_package(Qt5 5.15 REQUIRED COMPONENTS Core Gui Widgets Network WebSockets)
    set(QT_VERSION_MAJOR 5)
else()
    set(QT_VERSION_MAJOR 6)
endif()

# Source files
set(SOURCES
    src/main.cpp
    src/MainWindow.cpp
    src/canvas/CanvasView.cpp
    src/canvas/CanvasScene.cpp
    src/canvas/ImageItem.cpp
    src/canvas/TextItem.cpp
    src/canvas/SelectionRect.cpp
    src/network/SyncClient.cpp
    src/network/SyncServer.cpp
    src/network/CollabManager.cpp
    src/data/Board.cpp
    src/data/BoardSerializer.cpp
    src/ui/TitleBar.cpp
    src/ui/CursorWidget.cpp
    src/ui/ToolBar.cpp
)

set(HEADERS
    src/MainWindow.h
    src/canvas/CanvasView.h
    src/canvas/CanvasScene.h
    src/canvas/ImageItem.h
    src/canvas/TextItem.h
    src/canvas/SelectionRect.h
    src/network/SyncClient.h
    src/network/SyncServer.h
    src/network/CollabManager.h
    src/data/Board.h
    src/data/BoardSerializer.h
    src/ui/TitleBar.h
    src/ui/CursorWidget.h
    src/ui/ToolBar.h
)

# Resources
set(RESOURCES
    resources/resources.qrc
)

# Ensure resources directory structure exists for build
file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/resources/icons)

# Create executable
if(WIN32)
    add_executable(${PROJECT_NAME} WIN32 ${SOURCES} ${HEADERS} ${RESOURCES})
else()
    add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS} ${RESOURCES})
endif()

# Link Qt libraries
if(QT_VERSION_MAJOR EQUAL 6)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        Qt6::Network
        Qt6::WebSockets
        Qt6::OpenGLWidgets
    )
else()
    target_link_libraries(${PROJECT_NAME} PRIVATE
        Qt5::Core
        Qt5::Gui
        Qt5::Widgets
        Qt5::Network
        Qt5::WebSockets
    )
endif()

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Platform-specific settings
if(WIN32)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
elseif(APPLE)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_BUNDLE_NAME "CollabRef"
        MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    )
endif()

# Installation
install(TARGETS ${PROJECT_NAME}
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Copy server files for distribution
install(DIRECTORY server/ DESTINATION server)
