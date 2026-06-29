/*  dimmer.h — Cliente HTTP para enviar setpoint PPFD al DruidaBot
 *
 *  LADO DRUIDAVISION (este archivo): listo.
 *    Envía: GET http://<botIp>/api/dim?ppfd=<valor>
 *    Espera respuesta: HTTP 200 + JSON {"ok":true}
 *
 *  LADO DRUIDABOT (pendiente de implementar):
 *  ══════════════════════════════════════════════════════════════════════════
 *
 *  [1] ENDPOINT HTTP — agregar en el WebServer del DruidaBot (DruidaBot3.0.ino)
 *  ─────────────────────────────────────────────────────────────────────────
 *  En la sección de rutas del WebServer (donde están los otros server.on()):
 *
 *    server.on("/api/dim", HTTP_GET, []() {
 *      if (!server.hasArg("ppfd")) {
 *        server.send(400, "application/json", "{\"ok\":false,\"msg\":\"falta ppfd\"}");
 *        return;
 *      }
 *      int sp = server.arg("ppfd").toInt();
 *      if (sp < 0 || sp > 2000) {
 *        server.send(400, "application/json", "{\"ok\":false,\"msg\":\"ppfd fuera de rango\"}");
 *        return;
 *      }
 *      g_dimSetpointBot = sp;           // ver [2]
 *      g_dimAutoActivo  = true;         // activa el loop de control
 *      EEPROM.put(EEPROM_DIM_SETPOINT, g_dimSetpointBot);  // ver [3]
 *      EEPROM.put(EEPROM_DIM_ACTIVE,   (uint8_t)1);
 *      EEPROM.commit();
 *      server.send(200, "application/json", "{\"ok\":true}");
 *    });
 *
 *  ─────────────────────────────────────────────────────────────────────────
 *  [2] VARIABLES GLOBALES — agregar en config.h del DruidaBot
 *  ─────────────────────────────────────────────────────────────────────────
 *    int   g_dimSetpointBot = 500;  // PPFD objetivo (umol/m²·s)
 *    bool  g_dimAutoActivo  = false; // false = manual/apagado, true = loop activo
 *    float g_dimOutputPct   = 0.0f; // salida actual del dimmer (0.0 – 100.0 %)
 *
 *    // Módulo Waveshare 8CH 0-10V conectado al bus RS485
 *    // IMPORTANTE: asignarle un Modbus ID distinto a los sensores (IDs 1-7 ya usados).
 *    // Configurar el módulo físicamente en ID 10 (o el que corresponda).
 *    #define DIMMER_MODBUS_ID   10   // ID Modbus del Waveshare 8CH 0-10V
 *    #define DIMMER_CH_START     0   // registro del canal 1 (0x0000)
 *    #define DIMMER_CH_COUNT     8   // cantidad de canales a controlar
 *    #define DIMMER_MAX_VAL   1000   // valor = 1000 → 10.0V (escala del módulo)
 *    #define EEPROM_DIM_SETPOINT 1000  // int32_t — dirección libre en EEPROM
 *    #define EEPROM_DIM_ACTIVE   1004  // uint8_t
 *
 *  ─────────────────────────────────────────────────────────────────────────
 *  [3] DRIVER WAVESHARE 8CH 0-10V — función de escritura Modbus
 *  ─────────────────────────────────────────────────────────────────────────
 *  modbusWriteSingleReg() ya existe en DruidaBot. Úsala así:
 *
 *    // Escribe el mismo porcentaje en todos los canales (control unificado).
 *    // pct: 0.0 (apagado) – 100.0 (máximo brillo / 10V)
 *    void dimmerSetAll(float pct) {
 *      if (pct < 0.0f)   pct = 0.0f;
 *      if (pct > 100.0f) pct = 100.0f;
 *      uint16_t val = (uint16_t)(pct / 100.0f * DIMMER_MAX_VAL);
 *      for (uint8_t ch = 0; ch < DIMMER_CH_COUNT; ch++) {
 *        modbusWriteSingleReg(DIMMER_MODBUS_ID, DIMMER_CH_START + ch, val);
 *        delay(10);   // pequeña pausa entre escrituras al mismo bus RS485
 *      }
 *      g_dimOutputPct = pct;
 *    }
 *
 *    // Canal individual (para control independiente de zonas):
 *    void dimmerSetChannel(uint8_t ch, float pct) {
 *      if (ch >= DIMMER_CH_COUNT) return;
 *      if (pct < 0.0f)   pct = 0.0f;
 *      if (pct > 100.0f) pct = 100.0f;
 *      uint16_t val = (uint16_t)(pct / 100.0f * DIMMER_MAX_VAL);
 *      modbusWriteSingleReg(DIMMER_MODBUS_ID, DIMMER_CH_START + ch, val);
 *    }
 *
 *  ─────────────────────────────────────────────────────────────────────────
 *  [4] LOOP DE CONTROL PPFD → DIMMER — agregar en loop() del DruidaBot
 *  ─────────────────────────────────────────────────────────────────────────
 *  Control proporcional simple. Ejecutar cada 2-5 segundos.
 *  Usa g_ppfd (ya disponible en DruidaBot desde el sensor RS485).
 *
 *    // Agrega esta variable global junto a las otras:
 *    static uint32_t g_dimLastCtrl = 0;
 *
 *    // En loop(), después de leer g_ppfd:
 *    if (g_dimAutoActivo && (millis() - g_dimLastCtrl >= 3000)) {
 *      g_dimLastCtrl = millis();
 *
 *      if (!isfinite(g_ppfd) || !ppfdSensorOK) {
 *        // Sin lectura válida: no tocar el dimmer (seguridad)
 *        Serial.println("[DIM] sin lectura PPFD, dimmer congelado");
 *      } else {
 *        // Control proporcional: salida = setpoint / PPFD_MAX * 100
 *        // (ajusta PPFD_MAX al máximo de tu instalación, ej. 1500)
 *        const float PPFD_MAX = 1500.0f;
 *        float targetPct = (float)g_dimSetpointBot / PPFD_MAX * 100.0f;
 *
 *        // Opcional: corrección proporcional por error actual
 *        // float error = g_dimSetpointBot - g_ppfd;
 *        // float correction = error / PPFD_MAX * 20.0f;  // ganancia suave
 *        // targetPct = g_dimOutputPct + correction;
 *
 *        dimmerSetAll(targetPct);
 *        Serial.printf("[DIM] ppfd=%.0f sp=%d out=%.1f%%\n",
 *                      g_ppfd, g_dimSetpointBot, g_dimOutputPct);
 *      }
 *    }
 *
 *  ─────────────────────────────────────────────────────────────────────────
 *  [5] ESTADO MQTT — publicar el estado del dimmer con el resto del estado
 *  ─────────────────────────────────────────────────────────────────────────
 *  En la función que arma el JSON de estado (publishState o buildStateJson):
 *
 *    j += "\"dimmer\":{";
 *    j += "\"activo\":"   + String(g_dimAutoActivo  ? "true" : "false") + ",";
 *    j += "\"setpoint\":" + String(g_dimSetpointBot) + ",";
 *    j += "\"output\":"   + String(g_dimOutputPct, 1) + ",";
 *    j += "\"ppfd\":"     + (isfinite(g_ppfd) ? String((int)g_ppfd) : "null");
 *    j += "},";
 *
 *  ══════════════════════════════════════════════════════════════════════════
 */
#pragma once
#include <WiFi.h>
#include <WiFiClient.h>
#include <HTTPClient.h>
#include "store.h"   // g_botIp

// ── Centinelas de error previos al HTTP (no chocan con códigos HTTPClient) ──
#define DV_DIM_NO_IP    -2000   // g_botIp vacío — bot no emparejado
#define DV_DIM_NO_WIFI  -2001   // WiFi no conectado
#define DV_DIM_NO_BEGIN -2002   // HTTPClient::begin() falló

// ── Setpoint activo (RAM; se persiste en store.h via storeSaveDim) ──
int g_dimSetpoint = 500;         // umol/m²·s, rango útil 0–2000 (valor DESEADO)

// ── Última lectura PPFD EN VIVO del sensor (actualizada por la UI) ──
int g_dimLivePpfd = -1;          // umol/m²·s medido ahora; -1 = sin lectura aún

// ── Canal de salida activo en el módulo 0-10V (1-8; 0 = todos) ──
int g_dimChannel = 1;            // por defecto canal 1

// ── Flag para el loop principal (igual que g_uploadPending) ──
volatile bool g_dimPending = false;

// ── Resultado del último envío ──
int g_dimLastCode = 0;           // código HTTP o centinela; 0 = nunca enviado

// ─────────────────────────────────────────────────────────────────────────────
// dimmerSendSetpoint(livePpfd, setpoint)
//   Envía GET http://<botIp>/api/dim?ppfd=<livePpfd>&setpoint=<setpoint> al DruidaBot.
//   El bot usa "ppfd" como la MEDICION en vivo del Vision y "setpoint" como el valor
//   DESEADO (el bot lo aplica solo si su fuente de feedback es DruidaVision).
//   Retorna código HTTP (200 = aceptado) o centinela negativo.
//   Bloqueante — llamar solo desde el loop principal (no desde callbacks LVGL).
// ─────────────────────────────────────────────────────────────────────────────
int dimmerSendSetpoint(int setpoint) {
  if (g_botIp.length() == 0)         return DV_DIM_NO_IP;
  if (WiFi.status() != WL_CONNECTED) return DV_DIM_NO_WIFI;

  // Lectura en vivo: si aún no hay medición válida, mandamos el propio setpoint
  // como fallback para no romper el rango (el bot igual prioriza su fuente elegida).
  int live = (g_dimLivePpfd >= 0) ? g_dimLivePpfd : setpoint;

  WiFiClient cl;
  HTTPClient http;
  String url = "http://" + g_botIp + "/api/dim?ppfd=" + String(live)
             + "&setpoint=" + String(setpoint)
             + "&ch=" + String(g_dimChannel);
  if (!http.begin(cl, url))           return DV_DIM_NO_BEGIN;
  http.setTimeout(5000);
  int code = http.GET();
  http.end();
  return code;
}
