/*  dv_pins.h — Pines y constantes de la placa Waveshare ESP32-S3-Touch-LCD-2  */
#pragma once
#include <Arduino.h>

// ── LCD (SPI) ──
#define PIN_LCD_SCLK  39
#define PIN_LCD_MOSI  38
#define PIN_LCD_MISO  40
#define PIN_LCD_CS    45
#define PIN_LCD_DC    42
#define PIN_LCD_RST   0     // comparte con BOOT (validado: funciona)
#define PIN_LCD_BL    1

// ── Touch CST816 (I2C interno, lo administra LovyanGFX) ──
#define PIN_TP_SDA    48
#define PIN_TP_SCL    47
#define PIN_TP_INT    46

// ── TCS34725 (bus aparte Wire1) ──
#define PIN_TCS_SDA   17
#define PIN_TCS_SCL   18

// ── Bateria (ADC) ──
// Esquematico ETA6098: VBAT -200K- [BAT_ADC] -100K- GND  => Vbat = Vadc * 3 (GPIO5 = ADC1_CH4)
#define PIN_BAT_ADC   5

// ── Pantalla ──
static const uint16_t SCR_W = 240;
static const uint16_t SCR_H = 320;
