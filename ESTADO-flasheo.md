# Estado del flasheo USB — retomar acá (29-jun-2026)

Handoff para seguir mañana. Contexto completo en `CLAUDE.md` §Flasheo USB.

## Dónde quedamos

- Se libera el código para makers/devs → método de grabado **oficial: esptool** (carpeta
  `flash/`). El **flasher web ESP Web Tools quedó DEPRECADO** tras brickear una unidad
  (página live en mantenimiento).
- La unidad de prueba **se recuperó** (BOOT + Upload completo de Arduino).
- `esptool v5.3.1` ya instalado en esta PC (`py -m esptool`). Comando `write_flash` verificado.

## Lo que está hecho (sin testear en físico todavía)

1. **`flash/`** — método esptool listo:
   - `flash.bat` (Win) / `flash.sh` (Linux-mac): graban 4 partes
     (`bootloader`@0x0, `partitions`@0x8000, `boot_app0`@0xe000, `app`@0x10000).
   - `pack.bat`: refresca los bins desde `build/` tras *Export Compiled Binary*.
   - `README.md`: instrucciones (opción A binario / opción B compilar).
   - Los 4 bins están copiados (del build **anterior** a los cambios de firmware → NO tienen
     aún el botón "Actualizar por USB").
2. **Firmware (clon)** — agregado pero **sin recompilar/probar**:
   - `ui.h`: botón **Calib → "Actualizar por USB"** (con confirmación) → `g_dlModeRequested`.
   - `DruidaVision2free.ino`: `enterDownloadMode()` (`RTC_CNTL_FORCE_DOWNLOAD_BOOT` +
     `esp_restart()`), atendido en `loop()`. Reinicia a modo descarga de ROM sin botón BOOT.
3. **Web flasher** (`flasher/`): página en mantenimiento (pusheada). Manifest reescrito a
   4 partes por si se retomara, pero **deprecado**.

## TODO mañana (en orden)

### A) Validar esptool (rápido, sin recompilar)
1. Conectar placa, **mantener BOOT apretado**.
2. Correr `flash/flash.bat` → puerto = el COM estable de modo descarga (Enter = auto).
3. Debe grabar las 4 partes y arrancar. ➜ valida que el método esptool funciona.

### B) Firmware final con el botón nuevo
1. Arduino IDE → `DruidaVision2free.ino` → *Export Compiled Binary*.
   - ⚠ **Riesgo de compilación**: macro `RTC_CNTL_FORCE_DOWNLOAD_BOOT`. Si el core se queja,
     ver variante (otro nombre de registro / incluir `soc/rtc_cntl_reg.h` ya está puesto).
2. `flash/pack.bat` (refresca bins) → `flash/flash.bat` (graba).
3. Probar en el equipo: **Calib → Actualizar por USB → "Sí, actualizar"** → pantalla se apaga
   + aparece COM nuevo (USB JTAG/serial). Si aparece estable → entrada a bootloader SIN BOOT
   validada. Después `flash.bat` sin tocar BOOT.
4. Verificar: arranca free (3 pestañas, v2.0-free) y la calibración se mantiene.

## Decisiones / pendientes

- **HW (tuya):** pads **BOOT/EN** accesibles en la carcasa para des-brickear unidades muertas
  (la capa firmware solo sirve en equipos sanos). Imprescindible para venta masiva sin botón.
- **Limpieza al liberar:** decidir si se retira el repo `Pergah/druidavision-flasher` (web,
  deprecado) y armar un README de release en el repo principal `Pergah/DruidaVision2`.

## Gotchas aprendidos (no repetir)

- En USB nativo del S3, el **modo descarga es OTRO puerto COM** (cambia el número). Flashear
  al COM de la app (el que se cae en el loop) da "Cannot configure port".
- App-only / entrada por auto-reset desde la app corriendo = poco confiable → fue el brick
  (`invalid header: 0xffffffff` = app en blanco). Solución: entrada limpia (BOOT o el comando
  firmware) + flash completo.
