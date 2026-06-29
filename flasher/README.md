# DruidaVision Free — Flasher web por USB

Actualización del firmware por USB desde el navegador, vía
[ESP Web Tools](https://esphome.github.io/esp-web-tools/) (WebSerial + esptool-js).
Reemplaza al OTA en la versión free (que no tiene Config).

## ⚠ Historia / por qué este diseño

La 1ª versión (solo-app, 2 partes) **brickeó una unidad**: el flasher entraba al bootloader
por auto-reset *desde la app corriendo* y en esta placa de USB nativo esa entrada salía sucia
→ la app no se escribía (`invalid header: 0xffffffff`). La recuperación necesitó el botón
**BOOT** — que el producto final **no tiene**. Por eso el rediseño:

1. **Entrada limpia sin BOOT (firmware):** en el equipo, **Calib → Actualizar por USB**
   reinicia directo a modo descarga de ROM (`RTC_CNTL_FORCE_DOWNLOAD_BOOT`). Es el
   equivalente por software de apretar BOOT+RESET. **Hay que hacerlo antes de flashear.**
2. **Flash completo de 4 partes, sin borrar:** ver `manifest.json`.
3. **Recuperación de hardware:** pads BOOT/EN accesibles para des-brickear una unidad que ya
   no arranca (no puede recibir el comando del paso 1). **Decisión de hardware pendiente.**

## Qué escribe (`manifest.json`, `new_install_prompt_erase:false`)

| Parte | Offset | Qué es |
|---|---|---|
| `bootloader.bin` | `0x0` | bootloader 2ª etapa |
| `partitions.bin` | `0x8000` (32768) | tabla de particiones |
| `boot_app0.bin` | `0xE000` (57344) | otadata (bootea app0) |
| `DruidaVision2free.ino.bin` | `0x10000` (65536) | la app |

Es **exactamente lo que escribe el Upload de Arduino** (el que recuperó la unidad). Saltea
`nvs@0x9000` → **conserva la calibración** del usuario.

## Flujo de uso

1. En el equipo: **Calib → Actualizar por USB → “Sí, actualizar”** (la pantalla se apaga).
2. Conectar USB-C a la PC.
3. En `https://pergah.github.io/druidavision-flasher/` (Chrome/Edge): **Conectar y flashear**,
   elegir el puerto nuevo (USB JTAG/serial).
4. Termina y el equipo reinicia solo con el firmware nuevo.

## Publicar una versión

1. Arduino IDE → `DruidaVision2free.ino` → **Sketch → Export Compiled Binary**.
2. `..\publish_flasher.bat` → copia `bootloader.bin` + `partitions.bin` + `boot_app0.bin` +
   `.ino.bin` + la página al repo de Pages y pushea.
3. Si cambió la versión, actualizar `"version"` en `manifest.json` antes.

## ⚠ Estado actual: EN MANTENIMIENTO

La página live está deshabilitada (cartel de mantenimiento) hasta **re-testear el flujo nuevo
en una unidad con botones** (recuperable). No republicar el flasher funcional hasta validar:
entrar en modo USB desde Calib → flashear las 4 partes → arranca OK → calibración intacta.

## Archivos

- `index.html`, `manifest.json`, `boot_app0.bin` — fuente de la página.
- `bootloader.bin`, `partitions.bin`, `DruidaVision2free.ino.bin` — **no** están acá; los
  agrega `publish_flasher.bat` desde el `build/` de Arduino.
