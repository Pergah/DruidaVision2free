/*  battery.h — Lectura de bateria y estado de carga (placa ESP32-S3-Touch-LCD-2)
 *
 *  Hardware (confirmado en el esquematico oficial):
 *    - Cargador  : ETA6098 (Li-ion 1 celda). Su pin STAT solo prende el LED rojo
 *                  de carga; NO va a ningun GPIO. Tampoco hay sensado de VBUS.
 *    - Medicion  : VBAT --200K--[BAT_ADC=GPIO5]--100K--GND  =>  Vbat = Vadc * 3
 *
 *  Como NO hay senal de hardware de "cargando", los 3 estados se infieren por
 *  software desde la tension:
 *    - SIN BATERIA : el cargador, sin celda, oscila (ripple grande) a tension alta.
 *    - CARGANDO    : tension alta y estable (CV/lleno en USB) o subiendo (CC).
 *    - EN BATERIA  : resto (se muestra el % por la curva de la celda).
 *  Los umbrales DV_BAT_* son ajustables: la UI muestra la tension en pantalla
 *  (no hay Serial) para poder calibrarlos con lecturas reales.
 */
#pragma once
#include <Arduino.h>
#include <math.h>
#include "dv_pins.h"     // PIN_BAT_ADC

// Divisor del esquematico (200K/100K): Vbat = Vadc * 3
#define DV_BAT_DIVIDER   3.0f
// Curva LiPo 1 celda (aprox) para estimar el %
#define DV_BAT_FULL_mV   4200
#define DV_BAT_EMPTY_mV  3300
// Umbrales de deteccion de estado (ajustar segun lo que se vea en pantalla)
#define DV_BAT_FULL_TH   4180    // >= esto => cargando/lleno en USB
#define DV_BAT_RISE_TH     20    // mV que la EMA rapida supera a la lenta => subiendo (cargando)
#define DV_BAT_RIPPLE_TH  180    // mV de oscilacion (cargador ciclando sin celda) => sin bateria

// Bateria critica: a este % o menos (y sin estar cargando) se avisa "Recargar
// bateria" y se fuerza el reposo (ver uiShowLowBatterySplash / g_battCriticalSleep
// en ui.h, y el chequeo en el loop() del .ino) — el equipo no tiene boton de
// apagado fisico, asi que dormirlo es la unica forma de "apagarlo".
#define DV_BAT_LOW_PCT      4

// Tension minima para LEVANTAR la radio WiFi en el arranque. Por debajo de esto
// el pico de corriente de WiFi+backlight puede hundir el riel y disparar el
// brownout (reset). Si VBAT < esto, el boot deja la radio apagada (ver setup()).
#define DV_BAT_BOOT_MIN_mV  3450

// Bateria agotandose detectada en el arranque (ver powerCriticalHold en power.h):
// por debajo de esto NO se intenta el boot completo (evita el thrashing de
// brownout-resets del fin de bateria); se entra en espera de carga de minimo
// consumo. Al volver de esa espera (cargando), rebootea normal si VBAT supera
// DV_BAT_RESUME_mV. AJUSTAR con lecturas reales (la UI muestra la tension).
#define DV_BAT_CRIT_BOOT_mV 3300
#define DV_BAT_RESUME_mV    3500

enum DvPower { DV_PWR_BATT = 0, DV_PWR_CHARGING = 1, DV_PWR_NONE = 2 };

int     g_batMv    = 0;          // tension de bateria (mV, suavizada)
int     g_batPct   = 0;          // % estimado [0..100]
DvPower g_batState = DV_PWR_BATT;

#define DV_BAT_HIST 8
static int   s_batHist[DV_BAT_HIST];
static int   s_batHistN = 0, s_batHistI = 0;
static float s_batEmaFast = 0, s_batEmaSlow = 0;

// Lectura cruda promediada -> mV de bateria (ya multiplicado por el divisor)
static int batReadMv() {
  uint32_t acc = 0;
  const int N = 16;
  for (int i = 0; i < N; i++) acc += analogReadMilliVolts(PIN_BAT_ADC);
  return (int)lroundf((acc / (float)N) * DV_BAT_DIVIDER);
}

void batteryInit() {
  analogReadResolution(12);                 // 0..4095 (atenuacion por defecto ~11dB cubre 1.4V)
  int mv = batReadMv();
  g_batMv = mv;
  s_batEmaFast = s_batEmaSlow = (float)mv;
  for (int i = 0; i < DV_BAT_HIST; i++) s_batHist[i] = mv;
  s_batHistN = DV_BAT_HIST;
}

// Llamar periodicamente (cada ~1-2 s). Actualiza g_batMv / g_batPct / g_batState.
void batteryUpdate() {
  int mv = batReadMv();

  // EMA rapida (sigue la senal) y lenta (linea base de tendencia ~2 min @2s)
  s_batEmaFast += (mv - s_batEmaFast) * 0.25f;
  s_batEmaSlow += (mv - s_batEmaSlow) / 64.0f;
  g_batMv = (int)lroundf(s_batEmaFast);

  // historial corto para medir la oscilacion (ripple del cargador sin celda)
  s_batHist[s_batHistI] = mv;
  s_batHistI = (s_batHistI + 1) % DV_BAT_HIST;
  if (s_batHistN < DV_BAT_HIST) s_batHistN++;
  int mn = 999999, mx = 0;
  for (int i = 0; i < s_batHistN; i++) {
    int v = s_batHist[i];
    if (v < mn) mn = v;
    if (v > mx) mx = v;
  }
  int spread = mx - mn;

  // % por tension (clamp)
  int pct = (int)lroundf(100.0f * (g_batMv - DV_BAT_EMPTY_mV) /
                         (float)(DV_BAT_FULL_mV - DV_BAT_EMPTY_mV));
  g_batPct = constrain(pct, 0, 100);

  // estado
  bool rising = (s_batEmaFast - s_batEmaSlow) > DV_BAT_RISE_TH;
  if (spread > DV_BAT_RIPPLE_TH && g_batMv > 3700) {
    g_batState = DV_PWR_NONE;                // sin celda: el cargador oscila a tension alta
  } else if (g_batMv >= DV_BAT_FULL_TH || rising) {
    g_batState = DV_PWR_CHARGING;            // CV/lleno en USB, o subiendo (CC)
  } else {
    g_batState = DV_PWR_BATT;
  }
}
