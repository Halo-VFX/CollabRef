# CollabRef - Collaborative Reference Board

[![Build](https://github.com/YOUR_USERNAME/CollabRef/actions/workflows/build.yml/badge.svg)](https://github.com/YOUR_USERNAME/CollabRef/actions)

A PureRef-like application with real-time collaboration features. Drop images, arrange them on an infinite canvas, and collaborate with others in real-time.

## Download

**Pre-built binaries** (no compilation needed):
- [**Linux (x64)**](https://github.com/YOUR_USERNAME/CollabRef/releases/latest/download/CollabRef-Linux-x64.tar.gz)
- [**Windows (x64)**](https://github.com/YOUR_USERNAME/CollabRef/releases/latest/download/CollabRef-Windows-x64.zip)

Or go to [Releases](https://github.com/YOUR_USERNAME/CollabRef/releases) for all versions.

## Quick Start

### Linux
```bash
tar -xzf CollabRef-Linux-x64.tar.gz
cd CollabRef-Linux
./CollabRef.sh
```

### Windows
Extract the zip and run `CollabRef.exe`

**No installation required** - all libraries are bundled.

## Features

- **Infinite Canvas**: Pan and zoom with smooth navigation
- **Image Management**: Drag & drop, paste from clipboard, resize, rotate
- **Real-time Collaboration**: See other users' cursors and changes instantly
- **Text Annotations**: Add text notes to your board
- **GIF Support**: Animated GIFs play in the canvas
- **Zero Config**: Just run the app - first user hosts, others connect automatically
- **Persistent**: Board saves to disk, survives restarts
- **Frameless Window**: Minimal UI that stays out of your way
- **Always on Top**: Keep your references visible while working
- **Save/Load**: Save boards to `.cref` files

## Collaboration (Automatic!)

On the same server/network, everyone just runs:
```bash
./CollabRef.sh
```

- **First person** becomes the host automatically
- **Everyone else** connects automatically
- **Board saves** to `shared_board.json` next to the app
- **Persistent** - close and reopen anytime, your work is saved

No server setup, no configuration, no dependencies.

## Building from Source

### Prerequisites

#### Windows
1. **Qt 6.5+** (or Qt 5.15+): Download from [qt.io](https://www.qt.io/download-qt-installer)
   - Select: Qt 6.x > MSVC 2019 64-bit (or MinGW)
   - Select: Qt WebSockets
2. **CMake 3.16+**: Download from [cmake.org](https://cmake.org/download/)
3. **Visual Studio 2019/2022** (for MSVC) or **MinGW** compiler

#### macOS
```bash
brew install qt@6 cmake
```

#### Linux (Ubuntu/Debian)
```bash
sudo apt install qt6-base-dev qt6-websockets-dev cmake build-essential
```

### Build Steps

#### Windows (Visual Studio)

```batch
mkdir build
cd build
cmake -G "Visual Studio 17 2022" -A x64 -DCMAKE_PREFIX_PATH="C:/Qt/6.6.0/msvc2019_64" ..
cmake --build . --config Release
```

#### Windows (MinGW)

```batch
mkdir build
cd build
cmake -G "MinGW Makefiles" -DCMAKE_PREFIX_PATH="C:/Qt/6.6.0/mingw_64" ..
cmake --build .
```

#### macOS / Linux

```bash
mkdir build && cd build
cmake -DCMAKE_PREFIX_PATH=$(brew --prefix qt@6) ..
make -j$(nproc)
```

### Creating a Distributable Package (Windows)

After building, run Qt's deployment tool:

```batch
cd build\Release
windeployqt CollabRef.exe
```

This copies all required Qt DLLs next to the executable.

## Running the Collaboration Server

The collaboration server requires Node.js 16+.

```bash
cd server
npm install
npm start
```

The server runs on port 8080 by default. Set `PORT` environment variable to change it.

## Usage

### Basic Controls

| Action | Control |
|--------|---------|
| Pan | Middle-click drag or Space + Left-click drag |
| Zoom | Mouse wheel |
| Select | Left-click |
| Multi-select | Shift + Left-click or Marquee select |
| Move | Drag selected items |
| Rotate | Drag rotation handle (circle above selection) |
| Resize | Drag corner/edge handles |
| Delete | Delete key |
| Paste | Ctrl+V |
| Fit all | F |
| Reset view | R |
| Toggle always on top | T |

### Keyboard Shortcuts

| Shortcut | Action |
|----------|--------|
| Ctrl+N | New board |
| Ctrl+O | Open board |
| Ctrl+S | Save board |
| Ctrl+Shift+S | Save as |
| Ctrl+Z | Undo |
| Ctrl+Y | Redo |
| Ctrl+A | Select all |
| Delete | Delete selected |
| Escape | Deselect all |
| F | Fit all images in view |
| R | Reset view |
| 0 | Reset zoom to 100% |
| T | Toggle always on top |
| +/- | Zoom in/out |
| Arrow keys | Nudge selected (hold Shift for 10px) |

### Collaboration

On a shared server, everyone just runs the app - collaboration is automatic!

You can also manually host/join via **Right-click → Collaborate** if needed.

### File Format

CollabRef saves boards as `.cref` files, which are binary files containing:
- Board metadata (name, background color)
- All images embedded as PNG data
- Image positions, rotations, scales, and z-order

## Project Structure

```
CollabRef/
├── CMakeLists.txt          # Build configuration
├── src/
│   ├── main.cpp            # Application entry point
│   ├── MainWindow.cpp/h    # Main frameless window
│   ├── canvas/
│   │   ├── CanvasView.cpp/h    # QGraphicsView with pan/zoom
│   │   ├── CanvasScene.cpp/h   # QGraphicsScene managing items
│   │   ├── ImageItem.cpp/h     # Individual image items
│   │   └── SelectionRect.cpp/h # Marquee selection
│   ├── network/
│   │   ├── SyncClient.cpp/h    # WebSocket client
│   │   └── CollabManager.cpp/h # Collaboration logic
│   ├── data/
│   │   ├── Board.cpp/h         # Board data model
│   │   └── BoardSerializer.cpp/h # Save/load functionality
│   └── ui/
│       ├── TitleBar.cpp/h      # Custom title bar
│       ├── ToolBar.cpp/h       # Toolbar widgets
│       └── CursorWidget.cpp/h  # Remote cursor display
├── resources/
│   ├── resources.qrc       # Qt resources
│   └── icons/              # Application icons
```

## Troubleshooting

### Linux: "Permission denied"
```bash
chmod +x CollabRef.sh CollabRef
```

### Images appear blurry
This is normal when zoomed out. Zoom in to see full quality.

### Board not syncing
Right-click → Collaborate → Sync Now

## License

MIT License - see LICENSE file for details.

## Contributing

Contributions welcome! Please open an issue or pull request on GitHub.

## Credits

Inspired by [PureRef](https://www.pureref.com/), a fantastic reference tool.
Built with [Qt](https://www.qt.io/) and love.
