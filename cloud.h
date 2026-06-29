/*  cloud.h — Subida de capturas de PPFD a la webApp (HTTPS).
 *  Mismo patron que pushStateHttps() del DruidaBot: WiFiClientSecure + setInsecure.
 *  Postea bajo el device_id de un DruidaBot existente (campo Config).        */
#pragma once
#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <stdio.h>
#include "store.h"     // g_deviceId, g_botIp

// Centinelas para fallos previos al POST (no chocan con codigos de HTTPClient).
#define DV_CLOUD_NO_DEVID  -1000
#define DV_CLOUD_NO_WIFI   -1001
#define DV_CLOUD_NO_BEGIN  -1002

// Elimina caracteres que romperían un string JSON (comillas y backslash)
static void dvSanitizeJson(const char* src, char* dst, int maxLen) {
  int j = 0;
  if (src) for (int i = 0; src[i] && j < maxLen - 1; i++) {
    char c = src[i];
    if (c != '"' && c != '\\') dst[j++] = c;
  }
  dst[j] = '\0';
}

// Sube una captura. kind = "single" | "avg". location = nombre de zona (puede ser "").
// Devuelve el codigo HTTP (200 = OK) o un centinela DV_CLOUD_* si no se intentó el POST.
int cloudUpload(int ppfd, const char* kind, int samples, const char* location = "") {
  if (g_deviceId.length() == 0)        return DV_CLOUD_NO_DEVID;
  if (WiFi.status() != WL_CONNECTED)   return DV_CLOUD_NO_WIFI;

  WiFiClientSecure cl;
  cl.setInsecure();
  HTTPClient http;
  if (!http.begin(cl, "https://app.datadruida.com.ar/api/device/ppfd")) return DV_CLOUD_NO_BEGIN;
  http.addHeader("Content-Type", "application/json");
  http.setTimeout(10000);

  char locSafe[64] = "";
  dvSanitizeJson(location, locSafe, sizeof(locSafe));

  char body[256];
  snprintf(body, sizeof(body),
    "{\"device_id\":\"%s\",\"ppfd\":%d,\"kind\":\"%s\",\"samples\":%d,\"location\":\"%s\"}",
    g_deviceId.c_str(), ppfd, kind, samples, locSafe);

  int code = http.POST(body);
  http.end();
  Serial.printf("[CLOUD] POST ppfd=%d kind=%s n=%d loc='%s' -> HTTP %d\n",
                ppfd, kind, samples, locSafe, code);
  return code;
}

#define DV_PAIR_NO_FIELD   -2000   // el bot respondio pero sin "device_id" (firmware viejo)
#define DV_PAIR_BAD_ID     -2001   // respondio un device_id que no es un "druida_..." valido

// Emparejamiento LAN: GET http://<ip>/api/info y parsea "device_id".
// Devuelve el codigo HTTP (200 = OK y outId cargado) o un centinela DV_*.
int botFetchDeviceId(const String& ip, String& outId) {
  outId = "";
  if (ip.length() == 0)              return DV_CLOUD_NO_DEVID;   // falta IP
  if (WiFi.status() != WL_CONNECTED) return DV_CLOUD_NO_WIFI;    // sin WiFi

  WiFiClient cl;
  HTTPClient http;
  String url = "http://" + ip + "/api/info";
  if (!http.begin(cl, url)) return DV_CLOUD_NO_BEGIN;
  http.setTimeout(5000);

  int code = http.GET();
  if (code == 200) {
    String body = http.getString();
    int k = body.indexOf("\"device_id\"");
    if (k >= 0) {
      int c  = body.indexOf(':', k);
      int q1 = body.indexOf('"', c + 1);
      int q2 = (q1 >= 0) ? body.indexOf('"', q1 + 1) : -1;
      if (q1 >= 0 && q2 > q1) outId = body.substring(q1 + 1, q2);
    }
  }
  http.end();
  Serial.printf("[PAIR] GET %s -> HTTP %d  id='%s'\n", url.c_str(), code, outId.c_str());
  if (code == 200 && outId.length() == 0)          return DV_PAIR_NO_FIELD;  // respondio pero sin el campo
  if (code == 200 && !outId.startsWith("druida_"))  return DV_PAIR_BAD_ID;    // valor que no es un device_id
  return code;
}
