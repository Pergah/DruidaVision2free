#!/usr/bin/env bash
# DruidaVision Free - Flasheo por USB (esptool) - Linux / macOS
# Requiere esptool:  pip install esptool   (o el de Arduino/PlatformIO)
set -e
cd "$(dirname "$0")"

echo
echo " DruidaVision Free - Flasheo por USB (esptool)"
echo " ============================================="
echo

# Detectar esptool
if command -v esptool >/dev/null 2>&1; then ESPTOOL="esptool"
elif command -v esptool.py >/dev/null 2>&1; then ESPTOOL="esptool.py"
elif python3 -m esptool version >/dev/null 2>&1; then ESPTOOL="python3 -m esptool"
else
  echo " No se encontro esptool. Instalalo con:  pip install esptool"
  exit 1
fi

# Verificar binarios
for f in bootloader.bin partitions.bin boot_app0.bin DruidaVision2free.ino.bin; do
  [ -f "$f" ] || { echo " Falta el binario: $f"; exit 1; }
done

read -rp "Puerto serie (ej. /dev/ttyACM0 o /dev/cu.usbmodem*, Enter = autodetectar): " PORT
PORTARG=""
[ -n "$PORT" ] && PORTARG="-p $PORT"

echo
echo " Conecta la placa por USB. Si se queda en 'Connecting....':"
echo "   - manten apretado el boton BOOT, o"
echo "   - en el equipo: Calib -> Actualizar por USB."
echo

# shellcheck disable=SC2086
$ESPTOOL --chip esp32s3 $PORTARG -b 460800 write_flash \
  0x0     bootloader.bin \
  0x8000  partitions.bin \
  0xe000  boot_app0.bin \
  0x10000 DruidaVision2free.ino.bin

echo
echo " Listo! El equipo deberia reiniciar con el firmware nuevo."
echo " (Si no reinicia solo, apreta RESET o desenchufa/enchufa.)"
