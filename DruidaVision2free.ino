/*  DruidaVision v2 FREE — Medidor de PPFD tactil (version liberada/offline)
 *  Placa : Waveshare ESP32-S3-Touch-LCD-2 (ST7789 240x320 SPI + CST816 touch)
 *  Sensor: TCS34725 en Wire1 (SDA=17, SCL=18)
 *
 *  Variante FREE: solo pestañas Medir / DLI / Calib. SIN Config (WiFi/OTA/pairing),
 *  SIN pestaña Dim, SIN subida a la nube y SIN multiples mediciones (buffer/promedio).
 *  El codigo de net/web/cloud/ota/dimmer sigue compilando pero queda inerte (nada lo
 *  dispara: no hay UI que lo invoque y el WiFi arranca y permanece apagado).
 *
 *  ── Arduino IDE ──────────────────────────────────────────────────────────
 *  Librerias: LovyanGFX | lvgl 8.4.x | Adafruit_TCS34725 | Adafruit_Sensor
 *  Board "ESP32S3 Dev Module": PSRAM OPI | Flash 16MB | USB CDC On Boot ON
 *  lv_conf.h: LV_COLOR_DEPTH 16  (recomendado habilitar LV_FONT_MONTSERRAT_48)
 *
 *  Archivos (tabs): dv_pins.h | display.h | sensor.h | store.h | net.h | ui.h | web.h
 */

#include "display.h"   // LovyanGFX + glue LVGL
#include "sensor.h"    // TCS34725 + PPFD
#include "battery.h"   // bateria (ADC GPIO5) + estado de carga
#include "store.h"     // NVS (calibracion + red)
#include "net.h"       // WiFi AP/STA
#include "ota.h"       // actualizacion remota OTA (mismo repo que el bot)
#include "panels.h"    // base de calibracion por marca/modelo de panel LED (descargable)
#include "imu.h"       // QMI8658 acelerometro (nivel de burbuja)
#include "dimmer.h"    // cliente HTTP para setpoint PPFD → DruidaBot
#include "ui.h"        // pantallas
#include "web.h"       // portal web :80
#include "cloud.h"     // subida de capturas a la nube
#include "power.h"     // ahorro de bateria (reposo por inactividad)
#include "esp_system.h" // esp_reset_reason (motivo del ultimo reinicio)
#include "esp_attr.h"   // RTC_NOINIT_ATTR (vars que sobreviven al reset)
#include "esp_ota_ops.h" // esp_ota_set_boot_partition (rollback de OTA)
#include "soc/rtc_cntl_reg.h" // RTC_CNTL_OPTION1_REG (forzar modo descarga sin boton BOOT)

// FREE: reinicia DIRECTO al modo descarga de ROM (equivale a apretar BOOT+RESET, pero por
// software). Imprescindible en el producto sin boton: da la entrada LIMPIA al bootloader que
// el flasher web necesita (la entrada por auto-reset desde la app corriendo no es confiable
// en USB nativo). Un power-cycle real limpia el flag -> si se entra por error, basta
// desenchufar/enchufar para volver a arrancar normal. No retorna.
static void enterDownloadMode() {
  Serial.println("[USB] entrando a MODO DESCARGA por pedido del usuario");
  Serial.flush();
  delay(80);
  REG_WRITE(RTC_CNTL_OPTION1_REG, RTC_CNTL_FORCE_DOWNLOAD_BOOT);  // proximo arranque -> ROM download
  esp_restart();
}

// ── Anti-boot-loop (el equipo NO tiene boton: nunca debe quedar reiniciandose) ──
// Si se reinicia varias veces seguidas sin llegar a correr "sano" (brownout por
// bateria floja/mal contacto, o un crash por dato persistido corrupto), al
// N-esimo intento arranca en MODO SEGURO: sin radio WiFi (el mayor pico de
// corriente y un vector de crash) y sin leer el cache de paneles en NVS (posible
// dato corrupto). Asi SIEMPRE termina booteando, aunque sea sin WiFi, y el
// Serial deja registrado el motivo real del reinicio para atacar la causa de fondo.
#define DV_BOOT_MAGIC     0x44563242UL // "DV2B": valida que la RTC RAM no sea basura de power-on
#define DV_BOOT_LOOP_N    4            // reinicios seguidos (no power-on) -> modo seguro (auto-repara)
#define DV_BOOT_RECOVER_N 6            // ... y si ni en modo seguro arranca -> reset de fabrica
#define DV_BOOT_DIAG_N    8            // ... y si ni el reset de fabrica corta el loop -> modo diagnostico
#define DV_OTA_ROLLBACK_N 3            // crashes tras un OTA sin confirmar -> rollback al firmware anterior
RTC_NOINIT_ATTR static uint32_t s_bootMagic;   // sobreviven al reset (dominio RTC)
RTC_NOINIT_ATTR static uint32_t s_bootCount;
static bool     g_safeMode    = false;
static bool     g_bootHealthy = false;  // true tras correr unos segundos sin reiniciar
static uint32_t g_bootT0      = 0;

static const char* dv_resetReasonStr(esp_reset_reason_t r) {
  switch (r) {
    case ESP_RST_POWERON:   return "power-on";
    case ESP_RST_EXT:       return "reset externo";
    case ESP_RST_SW:        return "software";
    case ESP_RST_PANIC:     return "PANIC/crash";
    case ESP_RST_INT_WDT:   return "INT watchdog";
    case ESP_RST_TASK_WDT:  return "TASK watchdog";
    case ESP_RST_WDT:       return "watchdog";
    case ESP_RST_DEEPSLEEP: return "deep-sleep";
    case ESP_RST_BROWNOUT:  return "BROWNOUT (caida de tension)";
    default:                return "desconocido";
  }
}

// Rollback de OTA: si el firmware recien actualizado crashea en bucle, volvemos
// solos a la particion anterior (el firmware que venia funcionando). No depende de
// CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE (que el Arduino IDE no expone): lo hacemos
// a mano con esp_ota_set_boot_partition + el contador anti-loop. No retorna.
static void otaRollback() {
  const esp_partition_t* prev = esp_ota_get_next_update_partition(NULL);  // la pasiva = firmware anterior
  storeSaveOtaVerify(false);            // limpiar el flag ANTES de reiniciar (evita ping-pong)
  s_bootCount = 0;                      // el firmware viejo arranca "fresco"
  if (prev && esp_ota_set_boot_partition(prev) == ESP_OK)
    Serial.println("[OTA] firmware nuevo en bucle -> ROLLBACK al anterior");
  else
    Serial.println("[OTA] ROLLBACK: no hay particion previa valida");
  delay(50);
  esp_restart();
}

static uint32_t g_tRead  = 0;   // g_last vive en sensor.h (compartido)
static uint32_t g_tBatt  = 0;   // refresco lento del icono de bateria
static uint32_t g_tLevel = 0;   // refresco del indicador de nivel

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("\n=== DruidaVision v2 ===");

  esp_reset_reason_t rr = esp_reset_reason();

  // Lectura TEMPRANA de bateria (antes de encender pantalla/WiFi a full): nos dice
  // si este arranque viene de la bateria agotandose, no de un bug de software.
  batteryInit();                // ADC de bateria (GPIO5)
  bool battLow      = (g_batMv > 0 && g_batMv < DV_BAT_BOOT_MIN_mV);
  bool battCritical = (g_batMv > 0 && g_batMv < DV_BAT_CRIT_BOOT_mV);

  // Flag "OTA sin verificar", leido TEMPRANO: si el firmware nuevo crashea hasta en
  // el displayInit, igual podemos hacer rollback antes de tocar la pantalla.
  g_prefs.begin(DV_NS, true);
  g_otaVerify = g_prefs.getBool("ota_pend", false);
  g_prefs.end();

  // ── Anti-boot-loop, SIN contar los reinicios por energia ──
  // El equipo no tiene corte de bateria: al agotarse hace varios brownout-reset
  // (con menos brillo cada vez) hasta apagarse. Esos reinicios NO son crashes de
  // software y NO deben escalar a modo seguro / reset de fabrica -- si no,
  // perderiamos la config del usuario cada vez que se descarga la bateria. Solo
  // cuentan como posible crash-loop los reinicios con bateria sana y motivo != brownout.
  bool powerReset = (rr == ESP_RST_BROWNOUT) || battLow;
  if (s_bootMagic != DV_BOOT_MAGIC || rr == ESP_RST_POWERON) {
    s_bootMagic = DV_BOOT_MAGIC;        // arranque limpio (o RTC RAM con basura) -> contador a 0
    s_bootCount = 0;
  }
  if (!powerReset) s_bootCount++;
  g_safeMode = (s_bootCount >= DV_BOOT_LOOP_N);
  Serial.printf("[BOOT] motivo=%s vbat=%dmV intentos=%lu%s%s\n",
                dv_resetReasonStr(rr), g_batMv, (unsigned long)s_bootCount,
                powerReset ? " (por energia, no cuenta)" : "",
                g_safeMode ? "  => MODO SEGURO" : "");

  // Rollback de OTA: un firmware recien actualizado que crashea en bucle (bateria
  // sana, por eso cuenta) -> volver solo al firmware anterior. Va ANTES del modo
  // seguro/reset de fabrica: si el problema es el firmware nuevo, revertirlo es la
  // cura correcta (no borrar la config del usuario).
  if (g_otaVerify && s_bootCount >= DV_OTA_ROLLBACK_N) otaRollback();   // no retorna

  // Bateria agotandose: NO intentar el boot completo. Con la celda al limite, el
  // boot (splash a full + WiFi) dispara brownout tras brownout, drena lo que queda
  // y produce el "thrashing" de reinicios. En su lugar: aviso tenue + espera de carga
  // de minimo consumo (preserva TODA la config; reinicia al recuperar tension).
  if (battCritical) {
    Serial.println("[BOOT] bateria critica -> espera de carga (no retorna)");
    powerCriticalHold();
  }

  // Auto-reparacion (el usuario final NO puede borrar la flash por USB): ante un
  // crash REAL en bucle (con bateria sana), reset de fabrica de la config NVS como
  // ultimo recurso. Los paneles ya no son un vector: viven embebidos en el firmware.
  if (g_safeMode && s_bootCount >= DV_BOOT_RECOVER_N) {
    Serial.println("[BOOT] RECUPERACION DE FABRICA");
    storeFactoryReset();
  }

  // Ultimo recurso: si NI el reset de fabrica corta el loop, es un crash real e
  // independiente de la NVS (con bateria sana). No seguir thrasheando: pantalla de
  // diagnostico y espera (no retorna; un power-on real reintenta). Va antes de
  // displayInit/LVGL para escapar aunque el crash este en la inicializacion de la UI.
  if (g_safeMode && s_bootCount >= DV_BOOT_DIAG_N) {
    Serial.println("[BOOT] crash persistente -> MODO DIAGNOSTICO");
    powerDiagnosticHold(dv_resetReasonStr(rr), s_bootCount, g_batMv);
  }

  bool radioOff = g_safeMode || battLow;   // sin WiFi en este arranque (pico de corriente)
  if (battLow) dv_blAwake = DV_BL_SAVE;    // bateria baja: splash/sesion al 50% (menos brownout)

  displayInit();                // pantalla + LVGL (con splash, usa dv_blAwake)
  sensorInit();
  Serial.printf("[TCS] %s\n", g_tcsOK ? "OK (0x29)" : "NO detectado");

  imuInit();                     // QMI8658 (comparte Wire con el touch)
  storeInit();                  // carga calibracion + red desde NVS
  if (radioOff)   g_wifiEnabled = false;  // solo para ESTE arranque (no se persiste a NVS)
  panelsInit();                 // lista de paneles embebida en el firmware (no toca NVS/FFAT)
  panelsApplySelected();        // auto-resuelve la calib del panel elegido (se actualiza con cada OTA)
  Serial.printf("[NVS] factor=%.3f offset=%.1f ssid='%s' devid='%s'\n",
                g_factor, g_offset, g_ssid.c_str(), g_deviceId.c_str());
  displaySetSaver(g_powerSaveEnabled || radioOff);  // ahorro/seguro/bat-baja: 50% (menos consumo)

  netInit();                    // WiFi AP+STA (si g_wifiEnabled quedo false -> arranca apagado)
  Serial.printf("[NET] AP '%s' (%s)\n", g_apName.c_str(),
                WiFi.softAPIP().toString().c_str());

  uiInit();                     // tabview + pantallas
  uiSetWifi(g_wifiConnected);
  uiSetNetStatus(g_safeMode ? "MODO SEGURO - WiFi off"
               : battLow    ? "Bateria baja - WiFi off"
                            : netStatusStr().c_str());
  batteryUpdate();              // estado inicial de bateria
  uiSetBattery();

  webInit();                    // portal web :80
  g_bootT0 = millis();          // marca de tiempo para declarar el arranque "sano"
  Serial.println("[UI] lista");
}

void loop() {
  // Arranque "sano": tras correr unos segundos sin reiniciar, limpia el contador
  // anti-boot-loop para que un glitch aislado futuro no acumule hacia modo seguro.
  if (!g_bootHealthy && millis() - g_bootT0 > 8000) {
    g_bootHealthy = true;
    if (!g_safeMode) s_bootCount = 0;   // solo un arranque NORMAL sano limpia el contador;
                                        // en modo seguro lo dejamos para poder escalar a reset de fabrica
    if (g_otaVerify) {                  // el firmware nuevo arranco sano -> confirmar el OTA (cancela rollback)
      storeSaveOtaVerify(false);
      Serial.println("[OTA] firmware nuevo confirmado OK");
    }
  }

  if (g_dlModeRequested) {               // boton "Actualizar por USB" -> modo descarga (no retorna)
    g_dlModeRequested = false;
    lv_refr_now(NULL);                   // deja ver el cierre del popup antes de reiniciar
    enterDownloadMode();
  }

  if (powerLoop()) return;               // ahorro: en reposo duerme y no hace nada mas

  if (g_sleepRequested) {                // mantener 3s el PPFD/grafico -> reposo manual
    g_sleepRequested = false;
    powerEnterSleep();
    return;
  }

  if (g_battCriticalSleep) {             // termino el aviso de "bateria baja" -> dormir
    g_battCriticalSleep = false;
    powerEnterSleep();
    return;
  }

  lv_timer_handler();
  webLoop();                             // atiende el portal web :80

  if (netLoop()) {                       // cambio de estado WiFi
    uiSetWifi(g_wifiConnected);
    uiSetNetStatus(netStatusStr().c_str());
    Serial.printf("[NET] %s\n", netStatusStr().c_str());
  }

  String scanOpts;                       // resultado del scan async
  if (netScanCheck(scanOpts)) uiSetNetworks(scanOpts.c_str());

  if (g_pairPending) {                   // emparejar: leer device_id del bot por la LAN
    g_pairPending = false;
    lv_refr_now(NULL);                   // dibuja "Emparejando..." antes de bloquear
    String id;
    int code = botFetchDeviceId(g_botIp, id);
    if (code == 200 && id.startsWith("druida_")) {
      g_deviceId = id;                   // emparejado OK
      Serial.printf("[PAIR] OK device_id=%s\n", g_deviceId.c_str());
    } else {
      g_deviceId = "";                   // fallo: no dejar un id viejo/erroneo que de 404
    }
    storeSaveNet();                      // persiste el resultado (id valido o vacio) + botip
    uiPairResult(code, id.c_str());
  }

  if (g_uploadPending) {                 // subida a la nube (bloqueante, accion explicita)
    g_uploadPending = false;
    lv_refr_now(NULL);                   // dibuja "Subiendo..." antes de bloquear
    int val, samples;
    const char* kind;
    if (g_uploadMode == 1 && g_capN > 0) {
      val = (int)lround(g_capSum / g_capN);   // promedio de las muestras que tomo el usuario
      kind = "avg"; samples = g_capN;
    } else {
      val = (int)lroundf(g_last.ppfd);        // lectura actual (unica)
      kind = "single"; samples = 1;
    }
    int  code = cloudUpload(val, kind, samples, g_uploadLocation.c_str());
    bool ok   = (code == 200);
    uiCloudResult(code, val, kind, samples);
    if (ok && g_uploadMode == 1) uiClearBuffer();   // limpia el buffer tras subir el promedio
  }

  if (g_dimPending) {                     // envio de setpoint al dimmer (bloqueante, ~5s timeout)
    g_dimPending = false;
    lv_refr_now(NULL);                    // dibuja "Enviando..." antes de bloquear
    int code = dimmerSendSetpoint(g_dimSetpoint);
    g_dimLastCode = code;
    uiDimResult(code);
    Serial.printf("[DIM] setpoint=%d code=%d\n", g_dimSetpoint, code);
  }

  if (g_otaPending) {                    // actualizacion OTA (bloqueante, accion explicita)
    g_otaPending = false;
    lv_refr_now(NULL);                   // dibuja "Buscando firmware..." antes de bloquear
    int r = doOTAUpdate();               // descarga e instala; si sale OK reinicia (no retorna)
    uiOtaResult(r);                      // solo se llega aca si fallo o no habia cambios
  }

  if (!g_paused && (millis() - g_tRead >= 500)) {
    g_tRead = millis();
    sensorRead(g_last);
    uiUpdate(g_last);
    if (g_tcsOK) {
      Serial.printf("PPFD=%.1f CLEAR=%u | R%%=%.0f G%%=%.0f B%%=%.0f\n",
                    g_last.ppfd, g_last.clear, g_last.pr, g_last.pg, g_last.pb);
    }
  }

  if (millis() - g_tLevel >= 100) {       // indicador de nivel (10 Hz)
    g_tLevel = millis();
    float ax, ay, az;
    imuRead(ax, ay, az);
    // Referencia: dispositivo en portrait, pantalla mirando al usuario.
    // ax ~ +1g cuando vertical y nivelado.
    // roll  = inclinacion izq/der  (burbuja se mueve horizontalmente)
    // pitch = inclinacion fwd/back (burbuja se mueve verticalmente)
    float roll  = atan2f( ay,  ax) * (180.0f / M_PI);
    float pitch = atan2f( az,  ax) * (180.0f / M_PI);
    uiSetLevel(roll, pitch);
  }

  if (millis() - g_tBatt >= 2000) {      // icono de bateria (refresco lento)
    g_tBatt = millis();
    batteryUpdate();
    uiSetBattery();
    Serial.printf("[BAT] %d mV  %d%%  estado=%d\n", g_batMv, g_batPct, (int)g_batState);

    // Bateria critica (y no esta cargando): el equipo no tiene boton de apagado
    // fisico, asi que avisamos "Recargar bateria" (estilo celular) y, terminada
    // la animacion, nos dormimos solos (g_battCriticalSleep, ver mas arriba).
    // s_battLowArmed evita relanzar el aviso en cada refresco de 2s mientras se
    // muestra/duerme; se rearma solo si la bateria sube (se conecto el cargador).
    static bool s_battLowArmed = true;
    if (g_batState != DV_PWR_BATT || g_batPct > DV_BAT_LOW_PCT + 3) {
      s_battLowArmed = true;
    } else if (s_battLowArmed && g_batPct <= DV_BAT_LOW_PCT) {
      s_battLowArmed = false;
      Serial.println("[BAT] critica -> aviso + reposo");
      uiShowLowBatterySplash();
    }
  }

  delay(5);
}
