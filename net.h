/*  net.h — WiFi AP+STA con maquina de estados + scan async.
 *  El AP siempre disponible; STA se une a la red de casa cuando hay
 *  credenciales (necesario para la nube — paso 5).                          */
#pragma once
#include <WiFi.h>
#include <stdio.h>
#include "store.h"     // g_ssid, g_pass, g_deviceId

enum NetState { NET_AP, NET_CONNECTING, NET_CONNECTED, NET_FAILED, NET_OFF };

bool     g_wifiConnected = false;
String   g_apName        = "DruidaVision";
String   g_netStatusText = "-";
NetState g_netState      = NET_AP;
static uint32_t g_connStart = 0;
static bool     g_scanning  = false;

// Re-liga el portal web tras reactivar la radio WiFi (lo define web.h).
void webOnWifiUp();

// true si la pila de red (lwip/tcpip) esta inicializada, es decir la radio WiFi
// esta encendida (AP y/o STA). REGLA DE ORO: NO crear ni usar ningun socket si esto
// es false -- con la radio apagada el mutex de tcpip es NULO y cualquier operacion
// de socket dispara un assert (xQueueSemaphoreTake queue.c) -> PANIC en bucle. El
// portal web (web.h) se guarda con esto. cloud/dimmer/ota usan la condicion mas
// fuerte WiFi.status()==WL_CONNECTED (STA con IP), que ya implica netStackUp().
inline bool netStackUp() { return WiFi.getMode() != WIFI_MODE_NULL; }

static String dv_macSuffix() {
  uint8_t mac[6];
  WiFi.macAddress(mac);
  char b[8];
  snprintf(b, sizeof(b), "%02X%02X", mac[4], mac[5]);
  return String(b);
}

static void dv_beginSta() {
  if (g_ssid.length()) {
    WiFi.begin(g_ssid.c_str(), g_pass.c_str());
    g_netState  = NET_CONNECTING;
    g_connStart = millis();
  } else {
    g_netState = NET_AP;
  }
}

// (re)conecta el STA con g_ssid/g_pass
void netApply() {
  WiFi.disconnect();
  delay(50);
  dv_beginSta();
}

void netInit() {
  g_apName = "DruidaVision " + dv_macSuffix();    // MAC: disponible sin levantar la radio
  if (!g_wifiEnabled) {                           // arranca con WiFi apagado (ahorro)
    WiFi.mode(WIFI_OFF);
    g_netState      = NET_OFF;
    g_wifiConnected = false;
    g_netStatusText = "WiFi apagado";
    return;
  }
  WiFi.mode(WIFI_AP_STA);
  WiFi.setSleep(false);
  WiFi.softAP(g_apName.c_str());                 // AP abierto
  dv_beginSta();
  g_netStatusText = (g_netState == NET_CONNECTING)
                      ? ("Conectando a " + g_ssid + "...")
                      : ("AP: " + WiFi.softAPIP().toString());
}

// Enciende/apaga la radio WiFi (no persiste: eso lo hace storeSaveFlags desde la UI).
// Lo usan el switch de Config y el modo ahorro (apaga al dormir, restaura al despertar).
void netSetEnabled(bool on) {
  if (on) {
    if (!g_apName.length()) g_apName = "DruidaVision " + dv_macSuffix();
    WiFi.mode(WIFI_AP_STA);
    WiFi.setSleep(false);
    WiFi.softAP(g_apName.c_str());
    dv_beginSta();
    webOnWifiUp();                                // re-liga el socket del portal :80
    g_netStatusText = (g_netState == NET_CONNECTING)
                        ? ("Conectando a " + g_ssid + "...")
                        : ("AP: " + WiFi.softAPIP().toString());
  } else {
    WiFi.softAPdisconnect(true);
    WiFi.disconnect(true);                        // apaga la radio STA
    WiFi.mode(WIFI_OFF);
    g_wifiConnected = false;
    g_netState      = NET_OFF;
    g_netStatusText = "WiFi apagado";
  }
}

// Poll de estado (cada 500 ms). Devuelve true si cambio algo mostrable.
bool netLoop() {
  if (g_netState == NET_OFF) return false;        // radio apagada: nada que sondear
  static uint32_t t = 0;
  if (millis() - t < 500) return false;
  t = millis();

  bool     prevConn  = g_wifiConnected;
  NetState prevState = g_netState;
  g_wifiConnected = (WiFi.status() == WL_CONNECTED);

  if (g_wifiConnected) {
    g_netState = NET_CONNECTED;
  } else if (g_netState == NET_CONNECTED) {
    g_netState = NET_FAILED;                             // se cayo -> abajo se apaga la radio
  } else if (g_netState == NET_CONNECTING) {
    wl_status_t st = WiFi.status();
    if (st == WL_NO_SSID_AVAIL || st == WL_CONNECT_FAILED) g_netState = NET_FAILED;
    else if (millis() - g_connStart > 15000)              g_netState = NET_FAILED;
  }

  // El switch WiFi de Config representa "conectado a una red con internet": si la
  // conexion se cae o falla, se apaga la radio entera (ahorra bateria) y el switch
  // queda apagado (lo refleja uiSetWifi, que el .ino llama al ver que netLoop cambio
  // algo). El usuario vuelve a prenderlo cuando quiera reintentar.
  if (g_netState == NET_FAILED) {
    netSetEnabled(false);
    g_netStatusText = "No conecta (clave/SSID?) - WiFi apagado";
    g_wifiEnabled = false;
    storeSaveFlags();
    return true;
  }

  switch (g_netState) {
    case NET_CONNECTED:  g_netStatusText = "WiFi: " + WiFi.localIP().toString(); break;
    case NET_CONNECTING: g_netStatusText = "Conectando a " + g_ssid + "..."; break;
    default:             g_netStatusText = "AP: " + WiFi.softAPIP().toString(); break;
  }

  return (prevConn != g_wifiConnected) || (prevState != g_netState);
}

String netStatusStr() { return g_netStatusText; }

// ── Scan async de redes ──
void netScanStart() {
  if (g_scanning) return;
  WiFi.scanDelete();
  WiFi.scanNetworks(true);                       // async (no bloquea la UI)
  g_scanning = true;
}

// Devuelve true cuando el scan termino; out = "SSID1\nSSID2\n..." (vacio si no hay)
bool netScanCheck(String& out) {
  if (!g_scanning) return false;
  int n = WiFi.scanComplete();
  if (n == WIFI_SCAN_RUNNING) return false;
  g_scanning = false;
  out = "";
  if (n <= 0) { WiFi.scanDelete(); return true; }
  for (int i = 0; i < n && i < 20; i++) {
    String s = WiFi.SSID(i);
    if (s.length() == 0) continue;
    String hay = "\n" + out + "\n";
    if (hay.indexOf("\n" + s + "\n") >= 0) continue;     // dedupe
    if (out.length()) out += "\n";
    out += s;
  }
  WiFi.scanDelete();
  return true;
}
