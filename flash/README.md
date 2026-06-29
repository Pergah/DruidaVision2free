# Grabar DruidaVision Free por USB (esptool)

Método oficial de flasheo para makers/desarrolladores. Usa **esptool**, que es la misma
herramienta que usa Arduino por debajo: simple, robusta y multiplataforma.

> Placa: **Waveshare ESP32-S3-Touch-LCD-2** (ESP32-S3, 16 MB flash). Cualquier placa con el
> mismo chip/flash sirve compilando desde el código fuente.

## Opción A — Grabar el binario ya compilado (rápido)

1. Instalá esptool (una vez):
   ```
   pip install esptool
   ```
2. Conectá la placa por USB-C. Si se queda en `Connecting....`, mantené apretado **BOOT**
   (o en el equipo: **Calib → Actualizar por USB**, que lo deja en modo descarga sin botón).
3. Ejecutá el script de tu sistema:
   - **Windows:** doble clic en `flash.bat`
   - **Linux / macOS:** `bash flash.sh`

   Te pide el puerto (Enter = autodetectar) y graba las 4 partes.

### O a mano, en un comando
```
esptool --chip esp32s3 -p <PUERTO> -b 460800 write_flash \
  0x0     bootloader.bin \
  0x8000  partitions.bin \
  0xe000  boot_app0.bin \
  0x10000 DruidaVision2free.ino.bin
```
`<PUERTO>` = `COM5` (Windows) o `/dev/ttyACM0` (Linux) o `/dev/cu.usbmodemXXXX` (macOS).

### Alternativa: imagen única `merged.bin`
Si preferís un solo archivo (incluye todo, **borra la calibración** porque sobrescribe la
flash entera):
```
esptool --chip esp32s3 -p <PUERTO> write_flash 0x0 DruidaVision2free.ino.merged.bin
```
(El `merged.bin` se genera al compilar; pesa 16 MB. Por eso acá repartimos las 4 partes,
~1.6 MB.)

## Opción B — Compilar desde el código fuente (Arduino IDE)

1. Abrí `DruidaVision2free.ino` en Arduino IDE.
2. Board **ESP32S3 Dev Module**: PSRAM **OPI**, Flash **16MB**, USB CDC On Boot **ON**.
3. Partition Scheme **"16M Flash (3MB APP/9.9MB FATFS)"**.
4. **Upload** (usa esptool internamente).

## Notas

- Las 4 partes (`bootloader.bin` `0x0`, `partitions.bin` `0x8000`, `boot_app0.bin` `0xe000`,
  `DruidaVision2free.ino.bin` `0x10000`) son exactamente lo que escribe Arduino al subir.
- Para entrar a modo descarga sin botón BOOT, el firmware tiene **Calib → Actualizar por USB**
  (reinicia a modo descarga de ROM).
- Si el flasheo falla a 460800, bajá a `-b 115200`.
