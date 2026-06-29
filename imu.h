/*  imu.h — QMI8658 (acelerometro) para indicador de nivel
 *  Comparte Wire (I2C0, SDA=48 SCL=47) con el touch CST816D.
 *  Se accede solo desde el loop principal (single-task) → sin colision de bus. */
#pragma once
#include <Wire.h>
#include <math.h>
#include "dv_pins.h"

#define QMI_REG_WHO   0x00
#define QMI_REG_CTRL1 0x02
#define QMI_REG_CTRL2 0x03
#define QMI_REG_CTRL7 0x08
#define QMI_REG_AX_L  0x35

bool    g_imuOK   = false;
uint8_t g_imuAddr = 0x00;   // se detecta automaticamente (0x6A o 0x6B)
uint8_t g_imuWho  = 0xFF;   // valor leido de WHO_AM_I (para debug)

// Filtro EMA: amortigua vibraciones de mano sin perder respuesta
static float g_iax = 0.0f, g_iay = 0.0f, g_iaz = 1.0f;
static const float IMU_ALPHA = 0.25f;

static void qmi_write(uint8_t reg, uint8_t val) {
  Wire.beginTransmission(g_imuAddr);
  Wire.write(reg); Wire.write(val);
  Wire.endTransmission();
}

static uint8_t qmi_read1_at(uint8_t addr, uint8_t reg) {
  Wire.beginTransmission(addr);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) return 0xFF;
  Wire.requestFrom(addr, (uint8_t)1);
  return Wire.available() ? Wire.read() : 0xFF;
}

void imuInit() {
  // Wire puede haber sido inicializado ya por LovyanGFX con los mismos pines
  Wire.begin(PIN_TP_SDA, PIN_TP_SCL);
  delay(10);

  // Escanear el bus I2C para diagnostico y encontrar la direccion del QMI8658
  Serial.println("[IMU] Escaneando bus I2C (SDA=48 SCL=47)...");
  bool found = false;
  for (uint8_t addr = 0x08; addr < 0x78; addr++) {
    Wire.beginTransmission(addr);
    uint8_t err = Wire.endTransmission();
    if (err == 0) {
      Serial.printf("[IMU]   dispositivo en 0x%02X\n", addr);
      if (addr == 0x6A || addr == 0x6B) {
        g_imuAddr = addr;
        found = true;
      }
    }
  }

  if (!found) {
    Serial.println("[IMU] QMI8658 no encontrado en el bus (0x6A ni 0x6B)");
    return;
  }

  g_imuWho = qmi_read1_at(g_imuAddr, QMI_REG_WHO);
  Serial.printf("[IMU] QMI8658 addr=0x%02X WHO_AM_I=0x%02X\n", g_imuAddr, g_imuWho);

  // WHO_AM_I esperado: 0x05. Si difiere puede ser revision distinta; seguimos igual.
  qmi_write(QMI_REG_CTRL1, 0x60);  // I2C, auto-incremento de registro
  qmi_write(QMI_REG_CTRL2, 0x63);  // Accel: ±8g, ODR 125 Hz
  qmi_write(QMI_REG_CTRL7, 0x01);  // Solo acelerometro (ahorra corriente)

  g_imuOK = true;
  Serial.println("[IMU] QMI8658 OK");
}

// Lee el acelerometro con filtro EMA. Devuelve valores crudos en LSBs.
// Para el indicador de nivel usamos atan2 sobre los ratios → la escala no importa.
//
// AJUSTE DE EJES (si la burbuja se mueve al reves):
//   roll  usa ax y az  → negá ax para invertir horizontal
//   pitch usa ay y az  → negá ay para invertir vertical
//   Si roll y pitch estan intercambiados, swap ax <-> ay
void imuRead(float& ax, float& ay, float& az) {
  if (!g_imuOK) { ax = 0; ay = 0; az = 1; return; }

  uint8_t buf[6];
  Wire.beginTransmission(g_imuAddr);
  Wire.write(QMI_REG_AX_L);
  Wire.endTransmission(false);
  Wire.requestFrom(g_imuAddr, (uint8_t)6);
  for (int i = 0; i < 6; i++) buf[i] = Wire.available() ? Wire.read() : 0;

  int16_t rx = (int16_t)((buf[1] << 8) | buf[0]);
  int16_t ry = (int16_t)((buf[3] << 8) | buf[2]);
  int16_t rz = (int16_t)((buf[5] << 8) | buf[4]);

  g_iax = IMU_ALPHA * rx + (1.0f - IMU_ALPHA) * g_iax;
  g_iay = IMU_ALPHA * ry + (1.0f - IMU_ALPHA) * g_iay;
  g_iaz = IMU_ALPHA * rz + (1.0f - IMU_ALPHA) * g_iaz;

  ax = g_iax; ay = g_iay; az = g_iaz;
}
