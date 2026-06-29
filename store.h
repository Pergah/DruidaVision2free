/*  store.h — Persistencia en NVS (Preferences), namespace "druidavision"  */
#pragma once
#include <Preferences.h>
#include <math.h>
#include "sensor.h"     // g_factor, g_offset

// Declarado en dimmer.h; store.h lo persiste sin incluir el módulo completo.
extern int g_dimSetpoint;

static Preferences g_prefs;
static const char* DV_NS = "druidavision";

// Estado de red persistido
String g_ssid     = "";
String g_pass     = "";
String g_deviceId = "";   // device_id destino para la nube (se obtiene al emparejar)
String g_botIp    = "";   // IP local del bot a emparejar (de ahi se lee el device_id)

// Nombres de ubicación guardados (hasta DV_LOC_MAX); se persisten en NVS
#define DV_LOC_MAX 8
static char g_locSaved[DV_LOC_MAX][32] = {};  // hasta 8 nombres de 31 chars + NUL
static int  g_locCount = 0;                    // cuántos hay guardados actualmente
String g_uploadLocation = "";                  // ubicación activa para la próxima subida

// Preferencias de energia (persistidas)
// Por defecto: radio WiFi apagada y modo ahorro activado -> el equipo siempre
// arranca en el consumo mas bajo posible hasta que el usuario decida lo contrario.
bool g_wifiEnabled      = false;   // radio WiFi encendida (switch en Config)
bool g_powerSaveEnabled = true;    // ahorro de bateria: reposo tras inactividad

// Calibracion: panel elegido + flag 'custom'. El panel elegido permite re-resolver
// su factor/offset desde la lista EMBEBIDA en cada arranque, asi un OTA que mejore
// la calib de ese panel se aplica solo (ver panelsApplySelected en panels.h). Un
// ajuste manual (+/-) marca 'custom' para NO pisar lo que el usuario toco a mano.
String g_calPanelB = "";           // marca del panel elegido ("" = ninguno)
String g_calPanelM = "";           // modelo del panel elegido
bool   g_calCustom = false;        // true = calib ajustada a mano (ignora el panel)

// OTA sin verificar: lo setea el OTA antes de reiniciar; el firmware nuevo lo
// confirma al arrancar sano, o se hace rollback a la particion anterior si crashea
// en bucle (ver otaRollback en el .ino).
bool g_otaVerify = false;

// Carga calibracion + red desde NVS (con defaults/validacion como en v1)
void storeInit() {
  g_prefs.begin(DV_NS, true);                    // solo lectura
  g_factor   = g_prefs.getFloat("factor", 1.0f);
  g_offset   = g_prefs.getFloat("offset", 0.0f);
  g_calPanelB = g_prefs.getString("cal_pb", "");
  g_calPanelM = g_prefs.getString("cal_pm", "");
  g_calCustom = g_prefs.getBool  ("cal_cust", false);
  g_otaVerify = g_prefs.getBool  ("ota_pend", false);
  g_ssid     = g_prefs.getString("ssid", "");
  g_pass     = g_prefs.getString("pass", "");
  g_deviceId = g_prefs.getString("devid", "");
  g_botIp    = g_prefs.getString("botip", "");
  g_wifiEnabled      = g_prefs.getBool("wifi_en",  false);
  g_powerSaveEnabled = g_prefs.getBool("pwr_save", true);
  g_dimSetpoint      = g_prefs.getInt ("dim_sp",   500);   // setpoint PPFD para el dimmer
  // Nombres de ubicación guardados
  g_locCount = g_prefs.getInt("loc_n", 0);
  if (g_locCount < 0 || g_locCount > DV_LOC_MAX) g_locCount = 0;
  for (int i = 0; i < g_locCount; i++) {
    char key[8]; snprintf(key, sizeof(key), "loc%d", i);
    String v = g_prefs.getString(key, "");
    strncpy(g_locSaved[i], v.c_str(), 31);
    g_locSaved[i][31] = '\0';
  }
  g_uploadLocation = g_prefs.getString("loc_act", "");
  g_prefs.end();

  // Descarta device_id viejos invalidos (ej. una IP escrita a mano antes del emparejamiento).
  // Un device_id real siempre es "druida_" + MAC.
  if (g_deviceId.length() && !g_deviceId.startsWith("druida_")) g_deviceId = "";

  if (isnan(g_factor) || g_factor == 0.0f) g_factor = 1.0f;
  if (isnan(g_offset))                      g_offset = 0.0f;
}

// Borra TODA la config persistida (namespace NVS) y vuelve a defaults. Lo usa la
// recuperacion de fabrica del anti-boot-loop: si el equipo no arranca ni en modo
// seguro tras varios reinicios, se limpia el estado (ultimo recurso, sin USB).
void storeFactoryReset() {
  g_prefs.begin(DV_NS, false);
  g_prefs.clear();
  g_prefs.end();
  Serial.println("[NVS] factory reset (config borrada)");
}

// Marca/limpia el flag "OTA sin verificar" (lo usa el rollback de OTA en el .ino).
void storeSaveOtaVerify(bool v) {
  g_otaVerify = v;
  g_prefs.begin(DV_NS, false);
  g_prefs.putBool("ota_pend", v);
  g_prefs.end();
}

void storeSaveCal() {
  g_prefs.begin(DV_NS, false);
  g_prefs.putFloat("factor", g_factor);
  g_prefs.putFloat("offset", g_offset);
  g_prefs.end();
}

// Ajuste MANUAL de calibracion (+/-): persiste factor/offset y marca 'custom' para
// que un OTA NO pise lo que el usuario toco a mano (se desvincula del panel).
void storeSaveCalCustom() {
  g_calCustom = true;
  g_prefs.begin(DV_NS, false);
  g_prefs.putFloat("factor", g_factor);
  g_prefs.putFloat("offset", g_offset);
  g_prefs.putBool("cal_cust", true);
  g_prefs.end();
}

// Aplicar un PANEL: persiste factor/offset + el vinculo (marca/modelo) y limpia
// 'custom'. En cada arranque panelsApplySelected re-resuelve ese panel desde la
// lista embebida, asi un OTA que cambie su calib se aplica solo.
void storeSaveCalPanel(const String& b, const String& m) {
  g_calPanelB = b; g_calPanelM = m; g_calCustom = false;
  g_prefs.begin(DV_NS, false);
  g_prefs.putFloat("factor", g_factor);
  g_prefs.putFloat("offset", g_offset);
  g_prefs.putString("cal_pb", b);
  g_prefs.putString("cal_pm", m);
  g_prefs.putBool("cal_cust", false);
  g_prefs.end();
}

void storeSaveNet() {
  g_prefs.begin(DV_NS, false);
  g_prefs.putString("ssid",  g_ssid);
  g_prefs.putString("pass",  g_pass);
  g_prefs.putString("devid", g_deviceId);
  g_prefs.putString("botip", g_botIp);
  g_prefs.end();
}

// Guarda el setpoint del dimmer
void storeSaveDim() {
  g_prefs.begin(DV_NS, false);
  g_prefs.putInt("dim_sp", g_dimSetpoint);
  g_prefs.end();
}

// Guarda los nombres de ubicación y la ubicación activa
void storeSaveLocs() {
  g_prefs.begin(DV_NS, false);
  g_prefs.putInt("loc_n", g_locCount);
  for (int i = 0; i < g_locCount; i++) {
    char key[8]; snprintf(key, sizeof(key), "loc%d", i);
    g_prefs.putString(key, g_locSaved[i]);
  }
  g_prefs.putString("loc_act", g_uploadLocation.c_str());
  g_prefs.end();
}

// Guarda los switches de energia (WiFi on/off + ahorro de bateria)
void storeSaveFlags() {
  g_prefs.begin(DV_NS, false);
  g_prefs.putBool("wifi_en",  g_wifiEnabled);
  g_prefs.putBool("pwr_save", g_powerSaveEnabled);
  g_prefs.end();
}
