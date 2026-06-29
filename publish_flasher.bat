@echo off
setlocal EnableExtensions

echo.
echo  DruidaVision Free - Publicando flasher web (GitHub Pages)
echo  =========================================================
echo.

set "PROJECT_DIR=%~dp0"
set "BUILD_DIR=%PROJECT_DIR%build"
set "FLASHER_SRC=%PROJECT_DIR%flasher"
set "REPO_URL=https://github.com/Pergah/druidavision-flasher.git"
set "REPO_DIR=%USERPROFILE%\Documents\Arduino\druidavision-flasher"
set "APP_TARGET=DruidaVision2free.ino.bin"
set "PREFERRED_BIN=%BUILD_DIR%\esp32.esp32.esp32s3\DruidaVision2free.ino.bin"

REM --- 1) ubicar el .ino.bin del build (la app) ---
if not exist "%BUILD_DIR%" (
    echo  ERROR: no existe la carpeta build:
    echo  "%BUILD_DIR%"
    echo  Primero en Arduino IDE: Sketch -^> Export Compiled Binary
    goto error
)

if exist "%PREFERRED_BIN%" (
    set "SOURCE_BIN=%PREFERRED_BIN%"
) else (
    for /f "usebackq delims=" %%F in (`powershell -NoProfile -ExecutionPolicy Bypass -Command "Get-ChildItem -Path '%BUILD_DIR%' -Filter '*.ino.bin' -Recurse | Sort-Object LastWriteTime -Descending | Select-Object -First 1 -ExpandProperty FullName"`) do set "SOURCE_BIN=%%F"
)

if not defined SOURCE_BIN (
    echo  ERROR: no se encontro ningun archivo .ino.bin dentro de:
    echo  "%BUILD_DIR%"
    goto error
)

REM --- flash de 4 partes: tambien bootloader + tabla de particiones del mismo build ---
for /f "usebackq delims=" %%F in (`powershell -NoProfile -ExecutionPolicy Bypass -Command "Get-ChildItem -Path '%BUILD_DIR%' -Filter '*.bootloader.bin' -Recurse | Sort-Object LastWriteTime -Descending | Select-Object -First 1 -ExpandProperty FullName"`) do set "BOOTLOADER_BIN=%%F"
for /f "usebackq delims=" %%F in (`powershell -NoProfile -ExecutionPolicy Bypass -Command "Get-ChildItem -Path '%BUILD_DIR%' -Filter '*.partitions.bin' -Recurse | Sort-Object LastWriteTime -Descending | Select-Object -First 1 -ExpandProperty FullName"`) do set "PARTITIONS_BIN=%%F"

if not defined BOOTLOADER_BIN (
    echo  ERROR: no se encontro *.bootloader.bin en "%BUILD_DIR%"
    goto error
)
if not defined PARTITIONS_BIN (
    echo  ERROR: no se encontro *.partitions.bin en "%BUILD_DIR%"
    goto error
)

echo  App (build):  "%SOURCE_BIN%"
echo  Bootloader:   "%BOOTLOADER_BIN%"
echo  Particiones:  "%PARTITIONS_BIN%"
echo  Flasher src:  "%FLASHER_SRC%"
echo.

REM --- 2) clonar o actualizar el repo de Pages ---
if exist "%REPO_DIR%\.git" (
    echo  Actualizando repo local: "%REPO_DIR%"
    git -C "%REPO_DIR%" pull --rebase origin main
    if errorlevel 1 goto error
) else (
    echo  Clonando repo: %REPO_URL%
    if not exist "%REPO_DIR%" mkdir "%REPO_DIR%"
    git clone "%REPO_URL%" "%REPO_DIR%"
    if errorlevel 1 goto error
)

git -C "%REPO_DIR%" config user.name "Pergah"
git -C "%REPO_DIR%" config user.email "pergah@users.noreply.github.com"

REM --- 3) copiar pagina + manifest + bins al repo ---
echo.
echo  Copiando archivos del flasher...
copy /Y "%FLASHER_SRC%\index.html"    "%REPO_DIR%\index.html"      >nul
if errorlevel 1 goto error
copy /Y "%FLASHER_SRC%\manifest.json" "%REPO_DIR%\manifest.json"   >nul
if errorlevel 1 goto error
copy /Y "%FLASHER_SRC%\boot_app0.bin" "%REPO_DIR%\boot_app0.bin"   >nul
if errorlevel 1 goto error
copy /Y "%BOOTLOADER_BIN%"            "%REPO_DIR%\bootloader.bin"  >nul
if errorlevel 1 goto error
copy /Y "%PARTITIONS_BIN%"            "%REPO_DIR%\partitions.bin"  >nul
if errorlevel 1 goto error
copy /Y "%SOURCE_BIN%"                "%REPO_DIR%\%APP_TARGET%"     >nul
if errorlevel 1 goto error

REM --- 4) commitear si hay cambios y pushear ---
git -C "%REPO_DIR%" add -A
if errorlevel 1 goto error

set "STATUS_FILE=%TEMP%\druidavision_flasher_status.txt"
git -C "%REPO_DIR%" status --porcelain > "%STATUS_FILE%"
for %%A in ("%STATUS_FILE%") do set "STATUS_SIZE=%%~zA"

if "%STATUS_SIZE%"=="0" (
    echo.
    echo  No hay cambios para pushear. El flasher ya esta actualizado.
    goto success
)

echo.
echo  Creando commit y pusheando...
git -C "%REPO_DIR%" commit -m "Update flasher (firmware + pagina)"
if errorlevel 1 goto error
git -C "%REPO_DIR%" push origin main
if errorlevel 1 goto error

goto success

:error
echo.
echo  ERROR: algo salio mal. Revisa el log de arriba.
pause
exit /b 1

:success
echo.
echo  Listo. La pagina queda en: https://pergah.github.io/druidavision-flasher/
echo  (GitHub Pages puede tardar ~1 min en reflejar el cambio.)
echo  Presiona cualquier tecla para cerrar...
pause >nul
exit /b 0
