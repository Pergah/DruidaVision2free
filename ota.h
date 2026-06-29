/*  ota.h — Actualizacion remota OTA (igual patron que el DruidaBot)
 *
 *  Descarga el firmware nuevo desde el MISMO repo GitHub del DruidaBot
 *  (Pergah/DruidaBot3.0, rama main) pero con NOMBRE PROPIO para no chocar
 *  con el .bin del bot:   druidavision.ino.bin
 *      https://raw.githubusercontent.com/Pergah/DruidaBot3.0/main/druidavision.ino.bin
 *
 *  Publicar un release: en Arduino IDE  Sketch -> Export Compiled Binary,
 *  y despues correr  push_druidavision.bat  (copia el .ino.bin al repo,
 *  lo renombra a druidavision.ino.bin y lo pushea a main).
 *
 *  REQUISITO de build (Arduino IDE): particion con DOS apps OTA.
 *    Tools -> Partition Scheme -> "16M Flash (3MB APP/9.9MB FATFS)"
 *    (la misma que usa el bot: tiene app0/ota_0 + app1/ota_1).
 *    Con "Huge App" NO hay segunda particion y el OTA falla.
 *
 *  Sin Serial: el avance y el resultado se muestran en la pantalla OTA
 *  (barra de progreso + texto de estado).
 */
#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPUpdate.h>
#include <lvgl.h>          // lv_refr_now (refresco durante la descarga bloqueante)
#include "store.h"         // storeSaveOtaVerify (marca el OTA para verificar/rollback)

// Version de firmware instalada (subila a mano en cada release antes de compilar)
#define DV_FW_VERSION  "2.0-free"

// .bin en el MISMO repo del bot, con nombre distinto
#define DV_OTA_FIRMWARE_URL  "https://raw.githubusercontent.com/Pergah/DruidaBot3.0/main/druidavision.ino.bin"

// Resultado de la actualizacion (lo interpreta la UI)
enum DvOtaResult { DV_OTA_OK = 0, DV_OTA_NOUPD = 1, DV_OTA_FAIL = 2, DV_OTA_NOWIFI = 3 };

volatile bool g_otaPending = false;   // pedido de OTA desde la UI (lo atiende el loop)
int           g_otaErr     = 0;       // ultimo codigo de error (HTTP o lib Update)
String        g_otaErrStr  = "";      // descripcion del ultimo error

// Hooks de UI (definidos en ui.h). Forward-decl para poder llamarlos desde aca.
void uiOtaProgress(int pct);
void uiOtaSetMsg(const char* msg);

// Callback de progreso de httpUpdate -> refresca barra/% en pantalla (solo si cambia el %)
static void dv_otaProgressCb(int cur, int total) {
  static int last = -1;
  int pct = (total > 0) ? (int)((int64_t)cur * 100 / total) : 0;
  if (pct < 0) pct = 0; else if (pct > 100) pct = 100;
  if (pct != last) { last = pct; uiOtaProgress(pct); }
}

// Descarga e instala el firmware. BLOQUEANTE. Si sale OK reinicia (no retorna).
int doOTAUpdate() {
  if (WiFi.status() != WL_CONNECTED) return DV_OTA_NOWIFI;

  WiFiClientSecure client;
  client.setInsecure();                 // GitHub raw: sin validar cert (igual que el bot)

  httpUpdate.rebootOnUpdate(false);     // el reinicio lo manejamos nosotros
  httpUpdate.onProgress(dv_otaProgressCb);

  t_httpUpdate_return ret = httpUpdate.update(client, DV_OTA_FIRMWARE_URL);

  switch (ret) {
    case HTTP_UPDATE_OK:                 // ya dejo la nueva particion como booteable
      storeSaveOtaVerify(true);          // a verificar en el proximo boot (rollback si crashea)
      uiOtaSetMsg("OK! Reiniciando...");
      lv_refr_now(NULL);
      delay(900);
      // Apagado LIMPIO antes del reinicio. En ESP32-S3 (con PSRAM OPI) un
      // esp_restart() "en caliente" justo despues de escribir la flash del OTA,
      // con el WiFi todavia activo, a veces deja el chip en un estado del que el
      // proximo arranque no se recupera: crashea en bucle (rst 0xc / Saved PC en
      // IRAM) hasta un reset por HARDWARE. Bajar el radio y darle un respiro hace
      // que el arranque tibio se parezca a uno limpio y baja muchisimo esa chance.
      WiFi.disconnect(true);             // desconecta STA + apaga radio
      WiFi.mode(WIFI_OFF);
      delay(300);                        // respiro para que la flash/radio asienten
      ESP.restart();
      return DV_OTA_OK;                  // (no se alcanza)

    case HTTP_UPDATE_NO_UPDATES:         // el .bin del repo == el que corre (mismo MD5)
      return DV_OTA_NOUPD;

    case HTTP_UPDATE_FAILED:
    default:
      g_otaErr    = httpUpdate.getLastError();        // p.ej. 404 si falta el .bin
      g_otaErrStr = httpUpdate.getLastErrorString();
      return DV_OTA_FAIL;
  }
}
