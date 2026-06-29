/*  sensor.h — TCS34725 (Wire1) + conversion a PPFD  */
#pragma once
#include <Wire.h>
#include <math.h>
#include <Adafruit_TCS34725.h>
#include "dv_pins.h"

// Mismos settings que DruidaVision v1: la calibracion cubica depende de esto.
Adafruit_TCS34725 tcs(TCS34725_INTEGRATIONTIME_50MS, TCS34725_GAIN_1X);

bool  g_tcsOK  = false;
float g_factor = 1.0f;   // calibracion (se persistira en NVS — paso 3)
float g_offset = 0.0f;

struct Reading {
  uint16_t clear, r, g, b;
  float    ppfd;
  float    pr, pg, pb;     // % de espectro R/G/B
};

Reading g_last;            // ultima lectura (compartida con UI / web / nube)

// Calibracion cubica reusada de DruidaVision v1 (CLEAR -> PPFD)
inline float calcularPPFD(float clear) {
  float p = 2.15346803e-12f * clear * clear * clear
          - 5.69926833e-08f * clear * clear
          + 7.67401009e-02f * clear
          - 3.91116611f;
  if (p < 0.0f)    p = 0.0f;
  if (p > 1600.0f) p = 1600.0f;
  return p;
}

void sensorInit() {
  Wire1.begin(PIN_TCS_SDA, PIN_TCS_SCL);
  g_tcsOK = tcs.begin(TCS34725_ADDRESS, &Wire1);
}

void sensorRead(Reading& rd) {
  rd = Reading{};
  if (!g_tcsOK) return;

  uint16_t r, g, b, c;
  tcs.getRawData(&r, &g, &b, &c);
  rd.clear = c; rd.r = r; rd.g = g; rd.b = b;

  float p = calcularPPFD((float)c) * g_factor + g_offset;
  if (p < 0.0f) p = 0.0f;
  rd.ppfd = p;

  uint32_t tot = (uint32_t)r + g + b;
  rd.pr = tot ? 100.0f * r / tot : 0.0f;
  rd.pg = tot ? 100.0f * g / tot : 0.0f;
  rd.pb = tot ? 100.0f * b / tot : 0.0f;
}
