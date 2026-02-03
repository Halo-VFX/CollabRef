#!/bin/bash

echo "============================================"
echo "CollabRef Build Script"
echo "============================================"
echo

# Check for CMake
if ! command -v cmake &> /dev/null; then
    echo "ERROR: CMake not found"
    echo "Please install CMake:"
    echo "  macOS: brew install cmake"
    echo "  Ubuntu: sudo apt install cmake"
    exit 1
fi

# Detect Qt path
QT_PATH=""

# Check for Qt6 via brew (macOS)
if command -v brew &> /dev/null; then
    QT_BREW=$(brew --prefix qt@6 2>/dev/null || brew --prefix qt 2>/dev/null)
    if [ -d "$QT_BREW" ]; then
        QT_PATH="$QT_BREW"
    fi
fi

# Check common Linux paths
if [ -z "$QT_PATH" ]; then
    for path in /usr/lib/qt6 /usr/lib/x86_64-linux-gnu/qt6 /opt/Qt/6.*/gcc_64 ~/Qt/6.*/gcc_64; do
        if [ -d "$path" ]; then
            QT_PATH="$path"
            break
        fi
    done
fi

# Try to find qmake
if [ -z "$QT_PATH" ] && command -v qmake6 &> /dev/null; then
    QT_PATH=$(qmake6 -query QT_INSTALL_PREFIX)
elif [ -z "$QT_PATH" ] && command -v qmake &> /dev/null; then
    QT_PATH=$(qmake -query QT_INSTALL_PREFIX)
fi

if [ -z "$QT_PATH" ]; then
    echo "WARNING: Qt not found automatically."
    echo "Please set CMAKE_PREFIX_PATH manually."
    echo
    read -p "Enter Qt path (or press Enter to try system Qt): " QT_PATH
fi

echo "Using Qt at: ${QT_PATH:-system}"
echo

# Create build directory
mkdir -p build
cd build

# Configure
echo "Configuring with CMake..."
if [ -n "$QT_PATH" ]; then
    cmake -DCMAKE_PREFIX_PATH="$QT_PATH" -DCMAKE_BUILD_TYPE=Release ..
else
    cmake -DCMAKE_BUILD_TYPE=Release ..
fi

if [ $? -ne 0 ]; then
    echo
    echo "ERROR: CMake configuration failed"
    exit 1
fi

# Build
echo
echo "Building..."
cmake --build . --config Release -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

if [ $? -ne 0 ]; then
    echo
    echo "ERROR: Build failed"
    exit 1
fi

echo
echo "============================================"
echo "Build complete!"
echo "============================================"
echo
echo "Executable: $(pwd)/CollabRef"
echo
echo "To run: ./CollabRef"
