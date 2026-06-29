/*  power.h — Ahorro de bateria: reposo por inactividad.
 *
 *  Al activar el switch "Ahorro" (Config) el brillo baja al 50%. Ademas, si
 *  pasa mas de 1 minuto sin que se toque la pantalla, el equipo entra en REPOSO:
 *    - apaga el backlight (el mayor consumo de la placa),
 *    - apaga la radio WiFi,
 *    - duerme en light-sleep en ventanas cortas (~250 ms) y solo despierta a
 *      sondear el touch. Asi el micro pasa ~99% del tiempo dormido.
 *  Se REACTIVA con un toque en la pantalla. Al despertar restaura backlight y
 *  WiFi (segun la preferencia del usuario) y vuelve al funcionamiento normal.
 *
 *  El despertar es por TIMER (no por GPIO): en cada ventana sondea el touch por
 *  I2C. Es determinista (no puede quedar girando si el INT no tiene pull-up) y a
 *  prueba de cuelgues: el micro SIEMPRE vuelve a correr cada ~250 ms.
 */
#pragma once
#include <Arduino.h>
#include "esp_sleep.h"
#include "esp_system.h" // esp_restart (la espera de carga reinicia al recuperar tension)
#include "display.h"   // lcd, displayBacklight / displayTickPause / displayTickResume
#include "net.h"       // netSetEnabled
#include "store.h"     // g_wifiEnabled, g_powerSaveEnabled
#include "battery.h"   // batReadMv + umbrales (espera de carga por bateria critica)
// uiShowWakeSplash() vive en ui.h, incluido antes que este en el .ino (mismo TU Arduino)

#define DV_IDLE_MS      (60UL * 1000UL)       // 1 min sin tocar -> reposo
#define DV_SLEEP_US     (250000ULL)           // ventana de light-sleep (250 ms)
#define DV_WAKE_HOLD_MS (2000UL)              // mantener la pantalla tocada este rato (en reposo) -> despierta
#define DV_WAKE_TAP_GAP_MS (500UL)            // doble toque: max. tiempo entre el 1er y el 2do toque -> despierta

static bool     g_lowPower      = false;      // true mientras esta en reposo
static uint32_t s_wakeTouchSince = 0;         // 0 = sin contacto continuo; si no, millis() de cuando empezo
static uint32_t s_wakeLastTapAt  = 0;         // 0 = sin toque breve pendiente; si no, millis() de cuando se solto

// Entra en reposo: apaga pantalla + WiFi y detiene el tick de LVGL.
static void powerEnterSleep() {
  g_lowPower = true;
  s_wakeTouchSince = 0;                       // arranca limpio (ver dv_pause_cb: si entro con el dedo
  s_wakeLastTapAt  = 0;                       // todavia apoyado, debe sostenerlo 2s mas para despertar)
  displayBacklight(false);
  netSetEnabled(false);                       // radio WiFi apagada
  displayTickPause();                         // sin el tick de 1 ms el light-sleep dura
}

// Sale del reposo: enciende pantalla, restaura WiFi y se "traga" el toque que
// despierta para que no dispare un boton de la UI.
static void powerWake() {
  displayTickResume();                        // LVGL necesita su tick para crear/animar el splash
  uiShowWakeSplash();                         // arma el overlay (fondo opaco ya cubre la UI normal)
  lv_refr_now(NULL);                          // lo dibuja YA, antes de que se vea nada de la UI
  delay(30);                                  // margen para que el panel termine de volcar el splash a su GRAM
  displayBacklight(true);                     // recien ahora se enciende: lo primero que se ve es el splash
  uint16_t x, y; uint32_t t0 = millis();
  while (lcd.getTouch(&x, &y) && (millis() - t0) < 1500) delay(10);  // espera soltar
  lv_disp_trig_activity(NULL);                // reinicia el contador de inactividad de LVGL
  netSetEnabled(g_wifiEnabled);               // restaura WiFi segun preferencia
  g_lowPower = false;
}

// Llamar al principio del loop(). Devuelve true si esta en reposo (el loop debe
// saltarse entonces su trabajo normal: este bloquea en light-sleep ~250 ms).
bool powerLoop() {
  // Si esta dormido (sea por inactividad con "Ahorro" activado, sea por el
  // reposo manual de 3s o por bateria critica -- estos dos ultimos funcionan
  // SIN importar si "Ahorro" esta activado), siempre hay que sondear el touch
  // para poder despertarlo. Antes este chequeo estaba dentro del "if (g_powerSaveEnabled)"
  // de mas abajo, y al estar "Ahorro" desactivado el reposo manual se despertaba
  // solo, instantaneamente (entraba en powerEnterSleep y, en el mismo tick
  // siguiente, el otro branch lo despertaba de nuevo). Por eso va primero y
  // sin condicionar a g_powerSaveEnabled.
  if (g_lowPower) {
    esp_sleep_enable_timer_wakeup(DV_SLEEP_US);
    esp_light_sleep_start();                  // duerme ~250 ms
    uint16_t x, y;
    if (lcd.getTouch(&x, &y)) {
      // hay que sostener el toque DV_WAKE_HOLD_MS seguidos para despertar (evita que
      // un toque accidental, o el mismo dedo que recien lo durmio, lo reactive ya)
      if (!s_wakeTouchSince) s_wakeTouchSince = millis();
      else if (millis() - s_wakeTouchSince >= DV_WAKE_HOLD_MS) { powerWake(); return false; }
    } else {
      if (s_wakeTouchSince) {
        // se solto antes de llegar al HOLD -> fue un toque breve ("tap"). Si ya
        // habia uno reciente (dentro de DV_WAKE_TAP_GAP_MS), es un doble toque
        // -> despierta. Si no, lo deja pendiente como posible 1er toque.
        uint32_t now = millis();
        if (s_wakeLastTapAt && (now - s_wakeLastTapAt) <= DV_WAKE_TAP_GAP_MS) {
          powerWake(); return false;
        }
        s_wakeLastTapAt = now;
      }
      s_wakeTouchSince = 0;                  // se solto -> hay que empezar a contar de nuevo
    }
    return true;                             // sigue en reposo
  }
  // Reposo automatico por inactividad: solo si el usuario activo "Ahorro".
  if (g_powerSaveEnabled && lv_disp_get_inactive_time(NULL) > DV_IDLE_MS) {
    powerEnterSleep();
    return true;
  }
  return false;
}

// ── Espera de carga por bateria critica (detectada en el arranque) ──────────────
// El equipo no tiene corte fisico de bateria. Cuando la celda se agota, intentar el
// boot completo dispara brownout-reset repetidos (el "thrashing" con brillo cada vez
// menor que se ve hasta que el LED de PWR se apaga), drenando lo ultimo que queda.
// Aca, en cambio, mostramos un aviso muy tenue y entramos en minimo consumo:
//   - WiFi off, pantalla off, light-sleep en ventanas de 2 s;
//   - en cada ventana leemos VBAT; si subio (cargador conectado) reiniciamos para
//     bootear normal.
// NO se toca NADA de la config: un fin de bateria jamas debe borrar la calibracion,
// el WiFi ni el emparejamiento. Ademas, al dejar el consumo al minimo, el cargador
// puede sacar la celda de pre-carga y recuperarla (rompe el circulo vicioso de
// "no carga porque el equipo consume mas de lo que entra").
void powerCriticalHold() {
  lcd.init();
  lcd.setRotation(0);
  lcd.setBrightness(DV_BL_CRIT);
  lcd.fillScreen(lcd.color565(24, 8, 8));
  lcd.setTextColor(lcd.color565(255, 90, 90));
  lcd.setTextDatum(textdatum_t::middle_center);
  lcd.setTextSize(2);
  lcd.drawString("Recargar", SCR_W / 2, SCR_H / 2 - 18);
  lcd.drawString("bateria",  SCR_W / 2, SCR_H / 2 + 18);
  delay(1500);
  lcd.setBrightness(0);                        // pantalla off (el backlight es el mayor consumo)
  WiFi.mode(WIFI_OFF);                          // radio off (nunca se levanto en este arranque)
  for (;;) {
    esp_sleep_enable_timer_wakeup(2000000ULL); // ventana de 2 s
    esp_light_sleep_start();
    if (batReadMv() >= DV_BAT_RESUME_mV) esp_restart();  // cargando/recuperada -> boot normal
  }
}

// ── Modo diagnostico: ultimo recurso del anti-brick ─────────────────────────────
// Si ni el reset de fabrica corta el boot-loop, es un crash REAL e independiente de
// la NVS (con bateria sana): no hay dato persistido que borrar que lo cure, y seguir
// reiniciando solo thrashea y drena la celda. En vez de eso mostramos una pantalla
// legible con version + motivo + intentos (soporte / venta masiva, sin Serial en
// campo) y quedamos en espera, pantalla fija, sin reiniciar. Un power-on REAL
// (power-cycle, o recarga tras agotarse la bateria) resetea el contador anti-loop
// (s_bootMagic) y el equipo reintenta arrancar solo. Init propio del LCD (igual que
// powerCriticalHold): se llama ANTES de displayInit/LVGL, asi escapamos aunque el
// crash este en la inicializacion de la UI. NO retorna.
void powerDiagnosticHold(const char* motivo, uint32_t intentos, int vbat) {
  lcd.init();
  lcd.setRotation(0);
  lcd.setBrightness(DV_BL_SAVE);
  lcd.fillScreen(lcd.color565(10, 14, 30));
  lcd.setTextDatum(textdatum_t::middle_center);

  lcd.setTextColor(lcd.color565(0, 204, 255));
  lcd.setTextSize(2);
  lcd.drawString("FALLO DE", SCR_W / 2, 44);
  lcd.drawString("ARRANQUE", SCR_W / 2, 70);

  lcd.setTextSize(1);
  lcd.setTextColor(lcd.color565(180, 220, 255));
  char line[48];
  lcd.drawString("DruidaVision  v" DV_FW_VERSION, SCR_W / 2, 130);
  snprintf(line, sizeof(line), "Motivo: %s", motivo);
  lcd.drawString(line, SCR_W / 2, 158);
  snprintf(line, sizeof(line), "Intentos: %lu", (unsigned long)intentos);
  lcd.drawString(line, SCR_W / 2, 180);
  snprintf(line, sizeof(line), "Bateria: %d mV", vbat);
  lcd.drawString(line, SCR_W / 2, 202);

  lcd.setTextColor(lcd.color565(255, 180, 80));
  lcd.drawString("Reflashear por USB", SCR_W / 2, 250);
  lcd.drawString("o contactar soporte", SCR_W / 2, 270);

  setCpuFrequencyMhz(80);                      // ya dibujado: bajar reloj (radio off) -> menos consumo
  for (;;) delay(1000);                        // espera fija, no thrashea; power-on reintenta
}
