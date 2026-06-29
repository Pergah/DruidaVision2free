@echo off
REM Refresca los binarios de flasheo desde el ultimo build de Arduino.
REM Correr despues de: Arduino IDE -> Sketch -> Export Compiled Binary.
setlocal EnableExtensions
cd /d "%~dp0"

set "BUILD=..\build\esp32.esp32.esp32s3"
if not exist "%BUILD%" (
    echo  No existe "%BUILD%".
    echo  Primero en Arduino IDE: Sketch -^> Export Compiled Binary.
    pause & exit /b 1
)

copy /Y "%BUILD%\DruidaVision2free.ino.bootloader.bin" bootloader.bin            >nul && (
copy /Y "%BUILD%\DruidaVision2free.ino.partitions.bin" partitions.bin            >nul ) && (
copy /Y "%BUILD%\DruidaVision2free.ino.bin"            DruidaVision2free.ino.bin >nul )
if errorlevel 1 ( echo  ERROR copiando binarios. & pause & exit /b 1 )

echo  Binarios actualizados desde el build (boot_app0.bin es generico, no cambia).
echo  Ahora podes correr flash.bat
pause
