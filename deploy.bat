@echo off
setlocal enabledelayedexpansion

echo ============================================
echo CollabRef Deployment Script
echo ============================================
echo.

set "BUILD_DIR=Z:\CollabRef\build\Release"
set "VCPKG_DIR=Z:\CollabRef\vcpkg\installed\x64-windows"
set "DEPLOY_DIR=Z:\CollabRef\CollabRef-Portable"

:: Create clean deployment folder
echo Creating deployment folder...
if exist "%DEPLOY_DIR%" rmdir /S /Q "%DEPLOY_DIR%"
mkdir "%DEPLOY_DIR%"

:: Copy main executable
echo Copying executable...
copy "%BUILD_DIR%\CollabRef.exe" "%DEPLOY_DIR%\"

:: Copy Qt DLLs from vcpkg
echo Copying Qt DLLs...
copy "%VCPKG_DIR%\bin\Qt6Core.dll" "%DEPLOY_DIR%\"
copy "%VCPKG_DIR%\bin\Qt6Gui.dll" "%DEPLOY_DIR%\"
copy "%VCPKG_DIR%\bin\Qt6Widgets.dll" "%DEPLOY_DIR%\"
copy "%VCPKG_DIR%\bin\Qt6Network.dll" "%DEPLOY_DIR%\"
copy "%VCPKG_DIR%\bin\Qt6WebSockets.dll" "%DEPLOY_DIR%\"

:: Copy dependency DLLs
echo Copying dependency DLLs...
copy "%VCPKG_DIR%\bin\brotlicommon.dll" "%DEPLOY_DIR%\" 2>nul
copy "%VCPKG_DIR%\bin\brotlidec.dll" "%DEPLOY_DIR%\" 2>nul
copy "%VCPKG_DIR%\bin\bz2.dll" "%DEPLOY_DIR%\" 2>nul
copy "%VCPKG_DIR%\bin\freetype.dll" "%DEPLOY_DIR%\" 2>nul
copy "%VCPKG_DIR%\bin\harfbuzz.dll" "%DEPLOY_DIR%\" 2>nul
copy "%VCPKG_DIR%\bin\icudt*.dll" "%DEPLOY_DIR%\" 2>nul
copy "%VCPKG_DIR%\bin\icuin*.dll" "%DEPLOY_DIR%\" 2>nul
copy "%VCPKG_DIR%\bin\icuuc*.dll" "%DEPLOY_DIR%\" 2>nul
copy "%VCPKG_DIR%\bin\jpeg*.dll" "%DEPLOY_DIR%\" 2>nul
copy "%VCPKG_DIR%\bin\libpng*.dll" "%DEPLOY_DIR%\" 2>nul
copy "%VCPKG_DIR%\bin\pcre2*.dll" "%DEPLOY_DIR%\" 2>nul
copy "%VCPKG_DIR%\bin\zlib*.dll" "%DEPLOY_DIR%\" 2>nul
copy "%VCPKG_DIR%\bin\zstd.dll" "%DEPLOY_DIR%\" 2>nul
copy "%VCPKG_DIR%\bin\double-conversion.dll" "%DEPLOY_DIR%\" 2>nul
copy "%VCPKG_DIR%\bin\md4c.dll" "%DEPLOY_DIR%\" 2>nul
copy "%VCPKG_DIR%\bin\libcrypto*.dll" "%DEPLOY_DIR%\" 2>nul
copy "%VCPKG_DIR%\bin\libssl*.dll" "%DEPLOY_DIR%\" 2>nul

:: Copy all DLLs (to be safe)
echo Copying all bin DLLs...
copy "%VCPKG_DIR%\bin\*.dll" "%DEPLOY_DIR%\" 2>nul

:: Copy Qt plugins
echo Copying Qt plugins...
mkdir "%DEPLOY_DIR%\plugins\imageformats"
mkdir "%DEPLOY_DIR%\plugins\platforms"
mkdir "%DEPLOY_DIR%\plugins\styles"
mkdir "%DEPLOY_DIR%\plugins\tls"

copy "%VCPKG_DIR%\Qt6\plugins\imageformats\*.dll" "%DEPLOY_DIR%\plugins\imageformats\" 2>nul
copy "%VCPKG_DIR%\Qt6\plugins\platforms\*.dll" "%DEPLOY_DIR%\plugins\platforms\" 2>nul
copy "%VCPKG_DIR%\Qt6\plugins\styles\*.dll" "%DEPLOY_DIR%\plugins\styles\" 2>nul
copy "%VCPKG_DIR%\Qt6\plugins\tls\*.dll" "%DEPLOY_DIR%\plugins\tls\" 2>nul

:: Create qt.conf to tell Qt where plugins are
echo Creating qt.conf...
echo [Paths] > "%DEPLOY_DIR%\qt.conf"
echo Plugins = plugins >> "%DEPLOY_DIR%\qt.conf"

:: Copy Visual C++ runtime (if available)
echo.
echo NOTE: Users may need Visual C++ Redistributable installed.
echo Download from: https://aka.ms/vs/17/release/vc_redist.x64.exe
echo.

:: Create a simple README
echo Creating README...
(
echo CollabRef - Collaborative Reference Board
echo ==========================================
echo.
echo Just run CollabRef.exe to start!
echo.
echo Requirements:
echo - Windows 10 or later ^(64-bit^)
echo - Visual C++ Redistributable 2022
echo   Download: https://aka.ms/vs/17/release/vc_redist.x64.exe
echo.
echo Controls:
echo - Right-click: Menu
echo - Middle-click or Space+drag: Pan
echo - Scroll wheel: Zoom
echo - Drag and drop images onto window
echo - Ctrl+V: Paste image from clipboard
echo.
echo Collaboration:
echo 1. Right-click ^> Collaborate ^> Host Session
echo 2. Share the Room ID with others
echo 3. Others: Join Session with same Room ID
echo.
) > "%DEPLOY_DIR%\README.txt"

echo.
echo ============================================
echo Deployment complete!
echo ============================================
echo.
echo Folder: %DEPLOY_DIR%
echo.
echo To distribute:
echo 1. Zip the CollabRef-Portable folder
echo 2. Share the zip file
echo 3. Users just extract and run CollabRef.exe
echo.

:: Show folder size
echo Calculating size...
for /f "tokens=3" %%a in ('dir "%DEPLOY_DIR%" /-c /s ^| findstr /c:"File(s)"') do set SIZE=%%a
echo Total size: approximately %SIZE% bytes
echo.

pause
