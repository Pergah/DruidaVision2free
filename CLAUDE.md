# DruidaVision2 FREE — Medidor PPFD táctil (versión liberada)

> **⚠ Esta es la variante FREE/offline** (carpeta `DruidaVision2free`, sketch
> `DruidaVision2free.ino`, `DV_FW_VERSION = "2.0-free"`). Clon de `DruidaVision2`
> con un único delta funcional, todo en `ui.h`:
> - **Solo 3 pestañas: Medir / DLI / Calib.** No se construyen `Config`
>   (WiFi/OTA/pairing/ahorro) ni `Dim` (dimmer del bot) — sus builders
>   `dv_buildConfig`/`dv_buildDim` siguen definidos pero NO se invocan en `uiInit()`.
> - **Sin subida a la nube** (se quitó el icono/estado de nube y el botón "Subir" en Medir).
> - **Sin múltiples mediciones** (se quitó "Guardar muestra" + buffer/promedio); la vista
>   de controles del toggle queda solo como detalle RGB%.
> - El resto del firmware (anti-brick, batería, calibración por panel embebida, sensor,
>   nivel/IMU) queda **idéntico**. `net/web/cloud/ota/dimmer` siguen compilando pero
>   inertes (nada los dispara; WiFi arranca y permanece apagado).
>
> Para portar mejoras desde el proyecto pago: traer cambios de los demás `.h` tal cual;
> el único archivo divergente a conciliar es `ui.h`. El resto de este documento describe
> el firmware base completo.

## Flasheo USB

> **MÉTODO OFICIAL (open-source, makers/devs): `flash/` con esptool.** Se liberó el código
> para que makers armen su propio medidor PPFD; el que graba "mínimamente entiende", así que
> el método es **esptool** (lo que usa Arduino por debajo): robusto, multiplataforma, sin la
> fragilidad del WebSerial/USB-nativo que brickeó una unidad. `flash/flash.bat` (Win) /
> `flash/flash.sh` (Linux/mac) graban las **4 partes** (`bootloader`@0x0, `partitions`@0x8000,
> `boot_app0`@0xe000, `app`@0x10000) con `esptool write_flash`. También sirve compilar desde
> el fuente en Arduino IDE. Ver `flash/README.md`. Para entrar a modo descarga: botón BOOT de
> la placa, o **Calib → Actualizar por USB** (capa 1, abajo).
>
> **El flasher web (`flasher/`, ESP Web Tools) quedó DEPRECADO** tras el brick (página live en
> mantenimiento). Se conserva el historial abajo por las lecciones; no se usa.

### (Deprecado) Flasher web — historial y lecciones

Se intentó actualización por **USB desde el navegador** con
[ESP Web Tools](https://esphome.github.io/esp-web-tools/). Archivos fuente en `flasher/`.

**⚠ HISTORIA (brick real, 29-jun-2026):** la 1ª versión (solo-app, 2 partes: `boot_app0`+
`app`, sin borrar) **brickeó una unidad**. Síntoma: `invalid header: 0xffffffff` (app en
blanco) — la app NO se escribió pese a decir "OK". Causa: el flasher entraba al bootloader
por **auto-reset desde la app corriendo** y en esta placa de USB nativo esa entrada salía
sucia. Los offsets estaban bien (no fue eso). Recuperación: forzar modo descarga con **BOOT**
+ Upload completo de Arduino. **El producto no tiene BOOT** → hubo que reemplazarlo por
software. Rediseño en 3 capas:
- **Capa 1 — entrada limpia sin BOOT (firmware):** botón **Calib → "Actualizar por USB"**
  (`dv_updateUsb_cb`/`g_dlModeRequested` en `ui.h`; `enterDownloadMode()` en el `.ino` hace
  `REG_WRITE(RTC_CNTL_OPTION1_REG, RTC_CNTL_FORCE_DOWNLOAD_BOOT)` + `esp_restart()`) → reinicia
  directo a modo descarga de ROM. Equivale a BOOT+RESET. **Hay que hacerlo ANTES de flashear.**
  Un power-cycle real limpia el flag (si se entra por error, desenchufar/enchufar).
- **Capa 2 — flash de 4 partes, sin borrar:** `manifest.json` con `new_install_prompt_erase:
  false` escribe `bootloader.bin`@`0x0` + `partitions.bin`@`0x8000` + `boot_app0.bin`@`0xE000`
  + `.ino.bin`@`0x10000`. = exactamente el Upload de Arduino que recuperó la unidad. Saltea
  `nvs@0x9000` → **conserva la calibración**.
- **Capa 3 — recuperación de hardware (PENDIENTE, decisión de hardware):** pads **BOOT/EN**
  accesibles en la carcasa para des-brickear una unidad que ya no arranca (no puede recibir el
  comando de capa 1). Imprescindible para venta masiva sin botón.
- **Hosting:** repo `Pergah/druidavision-flasher`, GitHub Pages (`main`/root) →
  `https://pergah.github.io/druidavision-flasher/`. Chrome/Edge escritorio + HTTPS.
- **Publicar:** Arduino *Export Compiled Binary* → `publish_flasher.bat` (copia las 4 bins +
  la página al repo y pushea).
- **ESTADO: la página live está en MANTENIMIENTO** (deshabilitada) hasta re-testear el flujo
  nuevo en una unidad **con botones** (recuperable): Calib→Actualizar USB → flashear 4 partes
  → arranca OK → calibración intacta. NO republicar el flasher funcional hasta validar eso.

## Qué es
Reescritura del medidor PPFD (sensor **TCS34725**) sobre la placa Waveshare
**ESP32-S3-Touch-LCD-2** (LCD 2" 240x320 ST7789 SPI + touch CST816D, ESP32-S3R8 8MB
PSRAM/16MB flash). Reemplaza la v1 (ESP32 + OLED SSD1306). Mide PPFD/DLI, grafica el
espectro, permite capturar/subir lecturas a la nube y emparejarse con un DruidaBot.
Proyecto independiente del firmware del bot (`..\DruidaBot3.0`).
**Producto para venta masiva: el equipo NO tiene botón ni corte de batería → nunca
debe quedar brickeado ni reiniciándose** (ver "Blindaje anti-brick").

## Repos — IMPORTANTE (no confundir)
- **Código fuente** (`.ino`, `.h`, incluido `panels_data.h`) → repo `Pergah/DruidaVision2`
  (`origin` del clon local, rama `main`). Acá se commitea/pushea el firmware.
- **Binario OTA** → `Pergah/DruidaBot3.0` rama `main`, como `druidavision.ino.bin`
  (mismo repo donde el bot publica su `backend.ino.bin`; nombre distinto para no chocar).
  Lo sube `push_druidavision.bat`. NO mezclar fuente ahí.
- `..\druida-web-deploy`: webApp Vercel (`app.datadruida.com.ar`). Recibe capturas PPFD
  y es desde donde se publica la calibración de paneles al firmware.

## Stack
- LVGL **8.4.x** (NO v9) + **LovyanGFX** (driver ST7789 + CST816).
- Sensor `Adafruit_TCS34725` en bus **Wire1** (pines 17/18) para no chocar con el I2C del
  touch (47/48) que administra LovyanGFX.
- Predecesor de referencia (no tocar, solo consultar): `..\NuevoVision\NuevoVision.ino`.

## Build / carga
- Manual desde Arduino IDE (no hay build CLI ni CI). El asistente NO compila: el usuario
  flashea y prueba.
- **Requiere** Partition Scheme **"16M Flash (3MB APP/9.9MB FATFS)"** (dos particiones OTA,
  `app0/ota_0` + `app1/ota_1`). Con "Huge App" no hay 2da partición y el OTA/rollback fallan.
  (La partición FAT existe pero **el firmware ya NO la usa** — ver Blindaje.)
- Primera carga por USB con esa partición; las siguientes pueden ir por OTA.
- Release: `push_druidavision.bat` → clona/actualiza `..\DruidaBot_release`, copia
  `DruidaVision2.ino.bin` renombrado a `druidavision.ino.bin`, push a `main` de
  `Pergah/DruidaBot3.0`.
- Versión visible en pantalla (`DV_FW_VERSION`): se sube a mano por release.

## Calibración PPFD (cálculo base)
Modelo cúbico sobre canal **CLEAR** (depende de GAIN_1X + integración 50ms):
`2.15346803e-12*c³ - 5.69926833e-08*c² + 7.67401009e-02*c - 3.91116611`, clamp [0,1600],
luego `*factor + offset` (persistidos en NVS namespace `druidavision`).
DLI = `ppfd*3600*horas/1e6`.

## Calibración de paneles LED (EMBEBIDA, se actualiza con el OTA)
El catálogo marca/modelo → factor/offset **vive dentro del firmware**, NO se descarga.
- `panels.h` = structs + lógica (`panelsInit`, `panelsApplySelected`). Hecho a mano.
- **`panels_data.h` = GENERADO** (array `DV_PANELS_LIST` + `#define DV_PANELS_GEN`
  "<fecha> - N panel(es)", que se imprime en el boot para soporte). `panels.h` lo
  `#include` DESPUÉS de definir `struct DvPanelDef`. **No editar a mano.**
- Al "Aplicar" un panel se guarda el vínculo en NVS (`cal_pb`/`cal_pm`); en cada boot
  `panelsApplySelected()` re-resuelve factor/offset desde la lista embebida → un OTA que
  mejore la calib del panel elegido se aplica solo. Un ajuste manual +/- marca
  `cal_cust=true` y NO lo pisa el OTA.

**Flujo para actualizar calibraciones (web → firmware → equipos):**
1. Editar paneles en la webApp `/admin/paneles-led` (tabla Supabase `panel_calibrations`).
2. Botón **"⬆ Subir a GitHub"** → commitea `panels_data.h` a `Pergah/DruidaVision2`
   (ruta server-side `POST /api/admin/panel-calibrations/publish`, GitHub Contents API,
   requiere env `PANELS_GH_TOKEN` en Vercel). Alternativa manual: **"⬇ Exportar .h"** descarga
   el archivo para reemplazarlo a mano.
3. `git pull` en el firmware → Arduino IDE Export Compiled Binary → `push_druidavision.bat`.
4. Un solo OTA lleva **firmware + calibraciones** a los equipos.

(Histórico: antes la lista se descargaba de `/api/device/panels` y se cacheaba en FFAT
`/panels.json`. Ese endpoint fue **eliminado** y el FFAT también — era el vector de brick.)

## Blindaje anti-brick (venta masiva, equipo sin botón)
Surgió de un brick real (boot-loop por FFAT corrupto que ni el reflasheo arreglaba). Todo
apunta a que el equipo **siempre termine arrancando** sin perder la config del usuario.
- **FFAT eliminado por completo** (era el único vector de corrupción persistente). Los
  paneles ahora van embebidos. **NUNCA reintroducir FFAT/escritura a FAT.**
- **Anti-boot-loop auto-curativo** (`s_bootCount` en `RTC_NOINIT_ATTR`, sobrevive reset):
  `DV_BOOT_LOOP_N=4` reinicios seguidos → **MODO SEGURO** (WiFi off + brillo 50%);
  `DV_BOOT_RECOVER_N=6` → **reset de fábrica** (`storeFactoryReset()` borra NVS);
  `DV_BOOT_DIAG_N=8` → **modo diagnóstico** (`powerDiagnosticHold()` en `power.h`).
- **Modo diagnóstico = último recurso:** si ni el reset de fábrica corta el loop, el
  crash es REAL e **independiente de la NVS** (no hay dato persistido que borrar lo cure;
  seguir reiniciando solo thrashea/drena la celda). En vez de eso: pantalla mínima (init
  propio del LCD, **sin LVGL**, se llama ANTES de `displayInit` para escapar aunque el
  crash esté en la UI) con `FW/motivo/intentos/VBAT`, baja el reloj a 80MHz y queda en
  espera fija (no reinicia). Un **power-on real** (power-cycle / recarga tras agotarse)
  resetea `s_bootMagic` → reintenta solo. Para venta masiva sin botón: convierte un
  boot-loop invisible en un estado visible y soporteable.
- **El contador NO cuenta reinicios por energía** (`rr==ESP_RST_BROWNOUT` o
  `VBAT<DV_BAT_BOOT_MIN_mV=3450`): la descarga de batería hace varios brownout-reset y
  **no debe** escalar a modo seguro / borrar config. **Mantener este gating.**
- **Batería crítica al boot** (`VBAT<DV_BAT_CRIT_BOOT_mV=3300`): `powerCriticalHold()` en
  `power.h` — no intenta boot completo (evita thrashing), aviso tenue + pantalla/WiFi off +
  light-sleep, reinicia solo al recuperar `DV_BAT_RESUME_mV=3500`. **Umbrales a CALIBRAR
  con lecturas reales** (la UI muestra la tensión; no hay Serial en campo).
- **Rollback de OTA:** flag NVS `ota_pend` (`g_otaVerify`) que el OTA setea antes de
  reiniciar. El firmware nuevo: sano 8s → confirma (limpia el flag); crashea en bucle
  (`s_bootCount>=DV_OTA_ROLLBACK_N=3`, batería sana) → `otaRollback()` =
  `esp_ota_set_boot_partition(esp_ota_get_next_update_partition(NULL))` + restart → vuelve
  solo al firmware anterior. El flag se lee TEMPRANO en setup (antes de `displayInit`).
  Rollback va ANTES de modo seguro/reset de fábrica.
- **Warm-boot post-OTA (ESP32-S3 + PSRAM OPI):** un `esp_restart()` "en caliente" tras
  escribir flash con WiFi activo a veces deja el chip en un estado del que el próximo
  arranque no se recupera (boot-loop `rst 0xc` / Saved PC en IRAM) hasta un reset por
  HARDWARE. Fix en `ota.h` (HTTP_UPDATE_OK): `WiFi.disconnect(true); WiFi.mode(WIFI_OFF);
  delay(300); ESP.restart();`. Reduce la chance; el rollback es la red de seguridad.

## OTA
`ota.h` usa `httpUpdate` (mismo patrón que `doOTAUpdateFirmware()` del bot). El .bin se
descarga de `Pergah/DruidaBot3.0` main como `druidavision.ino.bin`. Botón chico en Config
(sin pestaña OTA dedicada); progreso/resultado reusan la línea de estado de Config.
404 (sin .bin) → `DV_OTA_FAIL` (no reinicia, no loop). Ver rollback + warm-boot arriba.

## Nube y emparejamiento
- **Emparejamiento por IP** (no por device_id manual): Config → IP local del DruidaBot →
  Emparejar → `GET http://<ip>/api/info` → parsea `"device_id"` → NVS (`g_deviceId`).
  Requiere el campo `device_id` en `/api/info` del bot (`handleApiInfo()`).
- **Captura de PPFD** (subida explícita por botón, buffer manual):
  `POST https://app.datadruida.com.ar/api/device/ppfd`
  `{"device_id","ppfd":<int>,"kind":"single"|"avg","samples":<int>}`.
  Tabla Supabase `ppfd_captures` + `GET /api/ppfd-captures` (repo `druida-web-deploy`).
- Diagnóstico en pantalla (sin Serial): `cloudUpload()` devuelve el código HTTP;
  `uiCloudResult()` lo traduce ("Error 404 (device_id?)", "Sin WiFi", etc.).

## Ahorro de energía
Switches en Config: WiFi on/off (`netSetEnabled`) y "Ahorro" (brillo 50% + light-sleep
tras 1 min de inactividad, despierta por toque). Indicador de batería por GPIO5/ADC
(divisor 200K/100K → `Vbat = Vadc×3`; sin señal HW de carga, se infiere por software).
**Defaults de fábrica (NVS vacía):** `g_powerSaveEnabled=true` y `g_wifiEnabled=false`
(`store.h`) — el equipo arranca siempre en el consumo más bajo posible; el usuario
prende WiFi/desactiva el ahorro a propósito. Solo aplica a equipos nuevos: si ya hay
flags persistidas se respetan.

### Switch WiFi = "conectado a internet" (`g_swWifi`, `ui.h`/`net.h`)
El switch representa el estado de conexión, no solo si la radio está prendida:
- **ON** → solo cuando `g_wifiConnected` (STA con IP, `uiSetWifi`). Al tocarlo el
  usuario prende la radio e intenta conectar (queda "Conectando..." sin tocar el
  switch hasta confirmar).
- **OFF** → radio apagada. Lo pone el usuario a mano (ahorrar batería / desconectarse
  a propósito) o lo hace `netLoop()` solo si la conexión falla o se cae
  (`NET_FAILED` → `netSetEnabled(false)` + `g_wifiEnabled=false` + `storeSaveFlags()`).
- "Buscar redes" (`dv_scan_cb`) y "Conectar WiFi" (`dv_connect_cb`) prenden la radio
  primero si estaba apagada (si no, el scan devolvía "(sin redes)" con la radio off).

**Gotcha (crash de boot por WiFi off):** con la radio apagada la pila lwip/tcpip NO
está inicializada; `g_server.begin()` / `handleClient()` (portal :80) toman un mutex
**NULO** → `assert xQueueSemaphoreTake queue.c` → PANIC en bucle. Por eso `web.h` NO
hace `begin()` en `webInit()` (solo registra handlers): `webLoop()` (re)liga `:80`
**solo** cuando `netStackUp()` (helper en `net.h`: `WiFi.getMode()!=WIFI_MODE_NULL`) y
no toca sockets si la radio está off. `webOnWifiUp()` (lo llama `netSetEnabled(true)`)
fuerza el re-`begin()` sobre el socket nuevo (apagar la radio destruye el anterior).
**Regla de oro: ningún socket/lwip con la radio apagada** (centralizada en `netStackUp()`)
— aplica también a cloud/dimmer/OTA, que usan la condición más fuerte `WL_CONNECTED`
(STA con IP, que ya implica `netStackUp()`). Este crash pegaba SOLO en equipos con NVS
limpia (de fábrica): arrancan con WiFi off por defecto → lwip abajo → `begin()` → assert.

## Bloqueo/reposo manual y despertar
Mantener presionado ≥3s (`DV_SLEEP_HOLD_MS`, `dv_sleepHold_cb` en `ui.h`) en cualquier
área "libre" (sin botón/widget clickable encima) de cualquier pestaña pide reposo manual
(`g_sleepRequested`). `dv_pageBase` cuelga ese handler del fondo de cada pestaña; LVGL
solo entrega el toque al clickable más profundo, así nunca dispara sobre un botón.
Para despertar (`power.h`, sondeo cada ~250ms en light-sleep): mantener tocado
`DV_WAKE_HOLD_MS` (2s) **o** dar dos toques rápidos seguidos (doble tap, gap
`DV_WAKE_TAP_GAP_MS` = 500ms).

## Gotcha LVGL (aplica también a DataBot)
`lv_label_set_text_fmt` **NO soporta `%f`** (LV_SPRINTF_USE_FLOAT=0 por defecto) → para
floats, formatear con `snprintf` a un buffer y usar `lv_label_set_text`. Los enteros
(`%d`/`%u`) sí funcionan con `_fmt`.

## Archivos clave
`DruidaVision2.ino` (setup/loop + anti-boot-loop + rollback), `dv_pins.h`+`display.h`
(pinout/driver LCD+touch), `sensor.h` (TCS34725 + calibración), `ui.h` (LVGL, tabs
Medir/DLI/Calib/Config), `net.h`/`web.h` (WiFi AP+STA / portal), `cloud.h` (subida nube +
pairing), `battery.h` (ADC + umbrales), `power.h` (ahorro/light-sleep + powerCriticalHold),
`store.h` (NVS + flags cal_cust/ota_pend), `ota.h`, `imu.h`, `panels.h` (lógica),
**`panels_data.h` (GENERADO)**, `dimmer.h`.

## Otros proyectos del ecosistema
- `..\DruidaBot3.0`: bot con el que se empareja (vía IP → `/api/info`).
- `..\druida-web-deploy`: capturas PPFD + publica calibración de paneles al firmware
  (`/admin/paneles-led` → "Subir a GitHub") + sirve el OTA (vía repo del bot).
