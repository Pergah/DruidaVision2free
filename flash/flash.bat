@echo off
setlocal EnableExtensions
cd /d "%~dp0"

echo.
echo  DruidaVision Free - Flasheo por USB (esptool)
echo  =============================================
echo.

REM --- Requiere esptool. Instalar con:  pip install esptool ---
set "ESPTOOL="
where esptool.exe >nul 2>nul && set "ESPTOOL=esptool.exe"
if not defined ESPTOOL ( python -m esptool version >nul 2>nul && set "ESPTOOL=python -m esptool" )
if not defined ESPTOOL ( py -m esptool version >nul 2>nul && set "ESPTOOL=py -m esptool" )
if not defined ESPTOOL (
    echo  No se encontro esptool. Instalalo con:
    echo      pip install esptool
    echo  ^(o usa el que viene con Arduino / PlatformIO^).
    pause & exit /b 1
)

REM --- Verificar binarios ---
for %%F in (bootloader.bin partitions.bin boot_app0.bin DruidaVision2free.ino.bin) do (
    if not exist "%%F" ( echo  Falta el binario: %%F & pause & exit /b 1 )
)

set "PORT="
set /p PORT=Puerto COM (ej. COM5, Enter = autodetectar):
set "PORTARG="
if defined PORT set "PORTARG=-p %PORT%"

echo.
echo  Conecta la placa por USB. Si se queda en "Connecting....":
echo    - manten apretado el boton BOOT, o
echo    - en el equipo: Calib -^> Actualizar por USB.
echo.

%ESPTOOL% --chip esp32s3 %PORTARG% -b 460800 write_flash ^
    0x0     bootloader.bin ^
    0x8000  partitions.bin ^
    0xe000  boot_app0.bin ^
    0x10000 DruidaVision2free.ino.bin

if errorlevel 1 (
    echo.
    echo  FALLO. Proba:
    echo    - bajar la velocidad: edita este .bat y cambia  -b 460800  por  -b 115200
    echo    - manten BOOT apretado durante "Connecting...."
    echo    - probar otro cable USB (de datos) y un puerto directo de la PC
    pause & exit /b 1
)

echo.
echo  Listo! El equipo deberia reiniciar con el firmware nuevo.
echo  (Si no reinicia solo, apreta RESET o desenchufa/enchufa.)
pause
