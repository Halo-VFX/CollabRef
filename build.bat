@echo off
setlocal enabledelayedexpansion

echo ============================================
echo CollabRef Build Script for Windows
echo ============================================
echo.

:: Check for CMake
where cmake >nul 2>nul
if %errorlevel% neq 0 (
    echo ERROR: CMake not found in PATH
    echo Please install CMake from https://cmake.org/download/
    pause
    exit /b 1
)

:: Try to find Qt
set QT_PATH=
set QT_PATHS=C:\Qt\6.6.0\msvc2019_64 C:\Qt\6.5.0\msvc2019_64 C:\Qt\6.4.0\msvc2019_64 C:\Qt\5.15.2\msvc2019_64

for %%p in (%QT_PATHS%) do (
    if exist "%%p\bin\qmake.exe" (
        set QT_PATH=%%p
        goto :found_qt
    )
)

:: Check common MinGW paths
set QT_PATHS_MINGW=C:\Qt\6.6.0\mingw_64 C:\Qt\6.5.0\mingw_64 C:\Qt\6.4.0\mingw_64

for %%p in (%QT_PATHS_MINGW%) do (
    if exist "%%p\bin\qmake.exe" (
        set QT_PATH=%%p
        set USE_MINGW=1
        goto :found_qt
    )
)

echo WARNING: Qt not found in common locations.
echo Please set QT_PATH manually or ensure Qt is installed.
set /p QT_PATH="Enter Qt path (e.g., C:\Qt\6.6.0\msvc2019_64): "

:found_qt
if not defined QT_PATH (
    echo ERROR: Qt path not set
    pause
    exit /b 1
)

echo Found Qt at: %QT_PATH%
echo.

:: Create build directory
if not exist build mkdir build
cd build

:: Configure with CMake
echo Configuring with CMake...
if defined USE_MINGW (
    cmake -G "MinGW Makefiles" -DCMAKE_PREFIX_PATH="%QT_PATH%" -DCMAKE_BUILD_TYPE=Release ..
) else (
    cmake -G "Visual Studio 17 2022" -A x64 -DCMAKE_PREFIX_PATH="%QT_PATH%" ..
)

if %errorlevel% neq 0 (
    echo.
    echo ERROR: CMake configuration failed
    pause
    exit /b 1
)

:: Build
echo.
echo Building...
if defined USE_MINGW (
    cmake --build . --config Release
) else (
    cmake --build . --config Release
)

if %errorlevel% neq 0 (
    echo.
    echo ERROR: Build failed
    pause
    exit /b 1
)

:: Deploy Qt DLLs
echo.
echo Deploying Qt libraries...

if defined USE_MINGW (
    set BUILD_DIR=.
) else (
    set BUILD_DIR=Release
)

if exist "%QT_PATH%\bin\windeployqt.exe" (
    "%QT_PATH%\bin\windeployqt.exe" --release --no-translations "%BUILD_DIR%\CollabRef.exe"
) else if exist "%QT_PATH%\bin\windeployqt6.exe" (
    "%QT_PATH%\bin\windeployqt6.exe" --release --no-translations "%BUILD_DIR%\CollabRef.exe"
)

echo.
echo ============================================
echo Build complete!
echo ============================================
echo.
echo Executable location: %CD%\%BUILD_DIR%\CollabRef.exe
echo.
echo To run, navigate to: %CD%\%BUILD_DIR%
echo.
pause
