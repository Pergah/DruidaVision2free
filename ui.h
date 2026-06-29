/*  ui.h — Pantallas LVGL: tabview (Medir / DLI / Calib / Config)
 *  Medir + DLI + Calibracion + Config(WiFi tactil) completas.
 *  El portal web (paso 4b) y la nube (paso 5) se agregan aparte.            */
#pragma once
#include <lvgl.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "display.h"
#include "sensor.h"
#include "store.h"
#include "net.h"
#include "battery.h"   // g_batMv / g_batPct / g_batState + DvPower
#include "ota.h"       // OTA: g_otaPending / doOTAUpdate / DV_FW_VERSION

// Fuente grande (PPFD) segun lo habilitado en lv_conf.h
#if LV_FONT_MONTSERRAT_48
  #define DV_PPFD_FONT &lv_font_montserrat_48
#elif LV_FONT_MONTSERRAT_44
  #define DV_PPFD_FONT &lv_font_montserrat_44
#elif LV_FONT_MONTSERRAT_40
  #define DV_PPFD_FONT &lv_font_montserrat_40
#elif LV_FONT_MONTSERRAT_36
  #define DV_PPFD_FONT &lv_font_montserrat_36
#elif LV_FONT_MONTSERRAT_28
  #define DV_PPFD_FONT &lv_font_montserrat_28
#else
  #define DV_PPFD_FONT LV_FONT_DEFAULT
#endif

// Fuente media (DLI)
#if LV_FONT_MONTSERRAT_28
  #define DV_MED_FONT &lv_font_montserrat_28
#elif LV_FONT_MONTSERRAT_24
  #define DV_MED_FONT &lv_font_montserrat_24
#elif LV_FONT_MONTSERRAT_22
  #define DV_MED_FONT &lv_font_montserrat_22
#elif LV_FONT_MONTSERRAT_20
  #define DV_MED_FONT &lv_font_montserrat_20
#else
  #define DV_MED_FONT LV_FONT_DEFAULT
#endif

// Paleta
#define DV_BG      lv_color_hex(0x0a0f1e)
#define DV_PANEL   lv_color_hex(0x111927)
#define DV_CYAN    lv_color_hex(0x00ccff)
#define DV_CYAN2   lv_color_hex(0x7bdfff)
#define DV_WHITE   lv_color_hex(0xffffff)
#define DV_GREY    lv_color_hex(0x55657a)

// ── Widgets vivos ──
static lv_obj_t* g_lblPpfd;
static lv_obj_t* g_lblPct;              // texto R/G/B % (solo en modo "controles")
static lv_obj_t* g_lblPause;            // icono Play/Pausa (entre el grafico y las pestañas): estado de la lectura en vivo
static lv_obj_t* g_lblCloudMsg;
static lv_obj_t* g_lblWifi;
static lv_obj_t* g_lblCloud;
static lv_obj_t* g_lblSensor;           // "S" roja arriba: sensor TCS no detectado
static lv_obj_t* g_lblBatt;             // glifo de bateria (nivel / rayo / vacio)
static lv_obj_t* g_lblBatVolt;          // tension chica al lado (diagnostico/calibracion)
static lv_obj_t* g_battSlash;           // linea diagonal (tachado) cuando no hay bateria
static lv_point_t g_battSlashPts[2];    // LVGL guarda el puntero -> debe persistir
// Espectro (canvas) + alternancia de vista grafico <-> controles
static lv_obj_t*   g_spec      = nullptr;   // canvas del grafico de espectro
static lv_color_t* g_specBuf   = nullptr;   // buffer del canvas (en PSRAM)
static lv_obj_t*   g_specAx[3] = {nullptr, nullptr, nullptr};  // etiquetas de eje (nm)
static lv_obj_t*   g_btnToggle;             // boton: alterna grafico <-> controles
static lv_obj_t*   g_lblToggle;             // icono del boton toggle
static lv_obj_t*   g_btnGuardar;            // controles (ocultos en modo grafico)
static lv_obj_t*   g_btnSubir;
static lv_obj_t*   g_btnClear;
static bool        g_showCtrls = false;     // false = grafico (def.), true = controles
static float       g_specWR = 1.0f;         // pesos de color (R/G/B norm. al dominante):
static float       g_specWG = 1.0f;         //   reforman la curva segun lo que mide el TCS
static float       g_specWB = 1.0f;         //   (1/1/1 = luz pareja => silueta de la foto)
// DLI
static lv_obj_t* g_rollerHours;
static lv_obj_t* g_lblDli;
static lv_obj_t* g_lblDliInfo;
// Calib
static lv_obj_t* g_lblFactor;
static lv_obj_t* g_lblOffset;
// Calib — menu "Paneles LED" (elegir marca/modelo -> aplica factor/offset)
static lv_obj_t* g_panelMenu;     // overlay del menu (oculto hasta abrirlo)
static lv_obj_t* g_ddBrand;       // desplegable de marcas
static lv_obj_t* g_ddModel;       // desplegable de modelos (segun la marca elegida)
static lv_obj_t* g_lblPanelMsg;   // estado / resultado ("Aplicado: ...", etc.)
// Config / WiFi
static lv_obj_t* g_taSsid;
static lv_obj_t* g_taPass;
static lv_obj_t* g_taIp;        // IP local del bot a emparejar
static lv_obj_t* g_swPair = nullptr; // switch de emparejamiento (ON=emparejado)
static lv_obj_t* g_swWifi = nullptr; // switch WiFi (ON=conectado a una red con internet)
static lv_obj_t* g_lblNetStatus;
static lv_obj_t* g_kb = NULL;
static lv_obj_t* g_ddNetworks;
static lv_obj_t* g_lblEye;
static bool      g_passShown = false;
// Panel de ingreso WiFi en horizontal (teclado ancho): al tocar un campo de Config se
// gira el display y se muestra este overlay con UN solo campo (el que se edita) grande
// arriba + teclado ancho abajo. Al confirmar se vuelca el valor al campo de Config.
static lv_obj_t* g_wifiKbScr    = nullptr; // overlay landscape (nullptr = cerrado)
static lv_obj_t* g_wifiKbField  = nullptr; // unico textarea visible (lo que se tipea)
static lv_obj_t* g_wifiKbEye    = nullptr; // glifo del ojo (solo para la clave)
static lv_obj_t* g_wifiKbKbd    = nullptr; // teclado del overlay
static lv_obj_t* g_wifiKbTarget = nullptr; // campo de Config que se esta editando
// OTA (actualizacion de firmware)
static lv_obj_t* g_lblFwVer;    // version instalada
static lv_obj_t* g_lblOta;      // estado / resultado de la actualizacion
static lv_obj_t* g_otaBar;      // barra de progreso de la descarga
// Dim (control dimmer via DruidaBot)
static lv_obj_t* g_lblDimLive;      // PPFD en vivo (pestaña Dim)
static lv_obj_t* g_lblDimSetpoint;  // valor del setpoint
static lv_obj_t* g_lblDimStatus;    // estado del último envío
static lv_obj_t* g_dimChLabel;      // label "CH N" dentro del botón selector de canal

// ── Popup de ubicación (para subida a la nube) ──
static lv_obj_t* g_locOverlay  = nullptr;   // overlay full-screen del popup
static lv_obj_t* g_locTextarea = nullptr;   // campo de texto con el nombre
static lv_obj_t* g_locBtn[8]   = {};        // botones del grid de nombres guardados
static int       g_locSelIdx   = -1;        // índice seleccionado (-1 = ninguno)

// ── Nivel ──
static lv_obj_t*  g_canvasLevel = nullptr;
static lv_color_t g_levelBuf[26 * 26];   // 1352 bytes en RAM interna (canvas 26x26)

bool        g_paused  = false;   // pausar/reanudar la lectura en vivo
static float g_uiPpfd = 0.0f;    // ultima PPFD mostrada (para DLI)
static int   g_dliHours = 12;
volatile bool g_uploadPending = false;  // pedido de subida a la nube (lo atiende el loop)
int           g_uploadMode    = 0;      // 0 = unica, 1 = promedio
int           g_capN   = 0;             // muestras guardadas por el usuario (buffer manual)
double        g_capSum = 0;             // suma de PPFD de esas muestras
volatile bool g_pairPending = false;    // pedido de emparejar con el bot (lo atiende el loop)
volatile bool g_sleepRequested = false; // mantener 3s el PPFD/grafico -> pedido de reposo manual (lo atiende el loop)
volatile bool g_battCriticalSleep = false; // termino el aviso de "bateria baja" -> pedido de reposo forzado (lo atiende el loop)
volatile bool g_dlModeRequested = false; // FREE: pidio "Actualizar por USB" -> el loop reinicia a modo descarga de ROM (sin boton BOOT)
// g_dimPending vive en dimmer.h (incluido antes que ui.h)

// ───────────────────────── helpers ─────────────────────────
#define DV_SLEEP_HOLD_MS  3000   // mantener presionado >= esto en area libre -> reposo manual

// Mantener presionado >= DV_SLEEP_HOLD_MS en cualquier area "libre" (sin un boton u
// otro widget clickable encima) pide el reposo manual (g_sleepRequested, lo atiende
// el loop llamando a powerEnterSleep). LVGL solo entrega el toque al objeto clickable
// mas profundo bajo el dedo, asi que esto nunca dispara presionando un boton: se cuelga
// del fondo de cada pestaña (dv_pageBase) y, ademas, del PPFD/grafico en Medir.
static void dv_sleepHold_cb(lv_event_t* e) {
  lv_event_code_t code = lv_event_get_code(e);
  static uint32_t pressStart = 0;
  static bool     sleepArmed = false;

  if (code == LV_EVENT_PRESSED) {
    pressStart = millis();
    sleepArmed = false;
    return;
  }
  if (code == LV_EVENT_LONG_PRESSED_REPEAT) {
    if (!sleepArmed && pressStart && (millis() - pressStart >= DV_SLEEP_HOLD_MS)) {
      sleepArmed = true;
      g_sleepRequested = true;          // el loop entra en reposo (powerEnterSleep)
    }
    return;
  }
  if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
    pressStart = 0;
  }
}

static void dv_pageBase(lv_obj_t* page) {
  lv_obj_set_style_bg_color(page, DV_BG, 0);
  lv_obj_set_style_bg_opa(page, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(page, 0, 0);
  lv_obj_set_style_pad_all(page, 0, 0);
  lv_obj_clear_flag(page, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_event_cb(page, dv_sleepHold_cb, LV_EVENT_ALL, NULL);  // toda el area libre de la pestaña
}

// muestra u oculta un objeto (helper para alternar grafico/controles)
static void dv_show(lv_obj_t* o, bool show) {
  if (!o) return;
  if (show) lv_obj_clear_flag(o, LV_OBJ_FLAG_HIDDEN);
  else      lv_obj_add_flag(o, LV_OBJ_FLAG_HIDDEN);
}

// boton pequeño +/- con texto
static lv_obj_t* dv_stepBtn(lv_obj_t* parent, const char* txt, lv_align_t al, int x, int y,
                            lv_event_cb_t cb, intptr_t tag) {
  lv_obj_t* b = lv_btn_create(parent);
  lv_obj_set_size(b, 48, 40);
  lv_obj_align(b, al, x, y);
  lv_obj_set_style_bg_color(b, DV_PANEL, 0);
  lv_obj_set_style_border_color(b, DV_CYAN, 0);
  lv_obj_set_style_border_width(b, 2, 0);
  lv_obj_add_event_cb(b, cb, LV_EVENT_CLICKED, (void*)tag);
  lv_obj_t* l = lv_label_create(b);
  lv_label_set_text(l, txt);
  lv_obj_set_style_text_color(l, DV_CYAN, 0);
  lv_obj_center(l);
  return b;
}

// boton con borde (sin label) — helper generico
static lv_obj_t* dv_borderBtn(lv_obj_t* page, int w, int h, lv_align_t al, int x, int y,
                              lv_event_cb_t cb) {
  lv_obj_t* b = lv_btn_create(page);
  lv_obj_set_size(b, w, h);
  lv_obj_align(b, al, x, y);
  lv_obj_set_style_bg_color(b, DV_PANEL, 0);
  lv_obj_set_style_border_color(b, DV_CYAN, 0);
  lv_obj_set_style_border_width(b, 2, 0);
  lv_obj_add_event_cb(b, cb, LV_EVENT_CLICKED, NULL);
  return b;
}

static void dv_placeholder(lv_obj_t* page, const char* txt) {
  dv_pageBase(page);
  lv_obj_t* l = lv_label_create(page);
  lv_label_set_text(l, txt);
  lv_obj_set_style_text_align(l, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_color(l, DV_CYAN2, 0);
  lv_obj_center(l);
}

// ───────────────────────── Medir ─────────────────────────

// Tap corto = pausar/reanudar la lectura en vivo. Mantener presionado >= 3s = pedir
// reposo manual (delegado a dv_sleepHold_cb). LVGL ya distingue tap de
// mantener-presionado con su propio long-press-time (~400ms por defecto): al
// superarlo no llega LV_EVENT_CLICKED, asi que no hay doble disparo.
static void dv_pause_cb(lv_event_t* e) {
  lv_event_code_t code = lv_event_get_code(e);
  dv_sleepHold_cb(e);
  if (code == LV_EVENT_PRESSED || code == LV_EVENT_LONG_PRESSED_REPEAT ||
      code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) return;
  if (code != LV_EVENT_CLICKED) return;

  static uint32_t lastPause = 0;
  uint32_t now = millis();
  if (now - lastPause < 400) return;     // debounce: evita doble disparo por tap
  lastPause = now;
  g_paused = !g_paused;
  // feedback: el numero PPFD deja de cambiar + icono y color reflejan el estado
  if (g_lblPause) lv_label_set_text(g_lblPause, g_paused ? LV_SYMBOL_PAUSE : LV_SYMBOL_PLAY);
  if (g_lblPpfd)  lv_obj_set_style_text_color(g_lblPpfd, g_paused ? DV_GREY : DV_WHITE, 0);
}

// muestra el estado del buffer manual en la linea de la nube
static void dv_showBuffer() {
  if (!g_lblCloudMsg) return;
  if (g_capN <= 0) { lv_label_set_text(g_lblCloudMsg, "Muestras: 0"); return; }
  char b[40];
  snprintf(b, sizeof(b), "Muestras: %d (prom %d)", g_capN, (int)lround(g_capSum / g_capN));
  lv_label_set_text(g_lblCloudMsg, b);
}

// Guardar: suma la lectura actual al buffer del usuario
static void dv_guardar_cb(lv_event_t* e) {
  g_capSum += g_uiPpfd;
  g_capN++;
  dv_showBuffer();
}

// Limpiar: descarta las muestras acumuladas
static void dv_clear_cb(lv_event_t* e) {
  g_capN = 0; g_capSum = 0;
  dv_showBuffer();
}

// ── Popup de ubicación — helpers y callbacks ─────────────────────────────────

// Actualiza el grid de botones de nombres guardados (tras guardar o borrar)
static void dv_locRefreshGrid() {
  for (int i = 0; i < DV_LOC_MAX; i++) {
    if (!g_locBtn[i]) continue;
    if (i < g_locCount) {
      lv_obj_t* lbl = lv_obj_get_child(g_locBtn[i], 0);
      if (lbl) lv_label_set_text(lbl, g_locSaved[i]);
      bool sel = (i == g_locSelIdx);
      lv_obj_set_style_bg_color(g_locBtn[i], sel ? DV_CYAN   : DV_PANEL, 0);
      lv_obj_set_style_bg_opa (g_locBtn[i], sel ? LV_OPA_30 : LV_OPA_COVER, 0);
      lv_obj_clear_flag(g_locBtn[i], LV_OBJ_FLAG_HIDDEN);
    } else {
      lv_obj_add_flag(g_locBtn[i], LV_OBJ_FLAG_HIDDEN);
    }
  }
}

// Toca un nombre guardado: lo pone en el textarea y lo marca como seleccionado
static void dv_locSel_cb(lv_event_t* e) {
  int idx = (intptr_t)lv_event_get_user_data(e);
  if (idx < 0 || idx >= g_locCount) return;
  g_locSelIdx = idx;
  if (g_locTextarea) lv_textarea_set_text(g_locTextarea, g_locSaved[idx]);
  dv_locRefreshGrid();
}

// Guarda el texto del textarea en la lista de nombres
static void dv_locSave_cb(lv_event_t* e) {
  if (!g_locTextarea) return;
  const char* txt = lv_textarea_get_text(g_locTextarea);
  if (!txt || txt[0] == '\0') return;
  // ¿ya existe idéntico?
  for (int i = 0; i < g_locCount; i++) {
    if (strncmp(g_locSaved[i], txt, 31) == 0) {
      g_locSelIdx = i; dv_locRefreshGrid(); return;
    }
  }
  if (g_locCount < DV_LOC_MAX) {
    strncpy(g_locSaved[g_locCount], txt, 31);
    g_locSaved[g_locCount][31] = '\0';
    g_locSelIdx = g_locCount++;
  } else {
    // lista llena: descarta el más viejo y agrega al final
    for (int i = 0; i < DV_LOC_MAX - 1; i++) memcpy(g_locSaved[i], g_locSaved[i+1], 32);
    strncpy(g_locSaved[DV_LOC_MAX-1], txt, 31);
    g_locSaved[DV_LOC_MAX-1][31] = '\0';
    g_locSelIdx = DV_LOC_MAX - 1;
  }
  storeSaveLocs();
  dv_locRefreshGrid();
}

// Borra el nombre actualmente seleccionado del grid
static void dv_locDel_cb(lv_event_t* e) {
  if (g_locSelIdx < 0 || g_locSelIdx >= g_locCount) return;
  for (int i = g_locSelIdx; i < g_locCount - 1; i++) memcpy(g_locSaved[i], g_locSaved[i+1], 32);
  g_locCount--;
  g_locSelIdx = -1;
  storeSaveLocs();
  dv_locRefreshGrid();
}

// Confirma la ubicación y dispara la subida a la nube
static void dv_locSend_cb(lv_event_t* e) {
  if (g_locTextarea) {
    const char* txt = lv_textarea_get_text(g_locTextarea);
    g_uploadLocation = String(txt ? txt : "");
    storeSaveLocs();  // persiste la ubicación activa para la próxima vez
  }
  lv_obj_del(g_locOverlay);
  g_locOverlay  = nullptr;
  g_locTextarea = nullptr;
  memset(g_locBtn, 0, sizeof(g_locBtn));
  g_uploadMode    = (g_capN > 0) ? 1 : 0;
  g_uploadPending = true;
  if (g_lblCloudMsg) lv_label_set_text(g_lblCloudMsg,
    (g_capN > 0) ? "Subiendo promedio..." : "Subiendo unica...");
}

// Cierra el popup sin subir
static void dv_locCancel_cb(lv_event_t* e) {
  lv_obj_del(g_locOverlay);
  g_locOverlay  = nullptr;
  g_locTextarea = nullptr;
  memset(g_locBtn, 0, sizeof(g_locBtn));
}

// ── Subir: abre el popup de ubicación en vez de subir directamente ──────────
static void dv_subir_cb(lv_event_t* e) {
  if (g_locOverlay) return;  // popup ya abierto

  static const int kbH  = 152;             // altura del teclado (px)
  const int        panH = LV_VER_RES - kbH; // 168 en pantalla de 320px

  // Overlay full-screen (fondo oscuro)
  lv_obj_t* ov = lv_obj_create(lv_scr_act());
  lv_obj_set_size(ov, LV_HOR_RES, LV_VER_RES);
  lv_obj_align(ov, LV_ALIGN_TOP_LEFT, 0, 0);
  lv_obj_set_style_bg_color(ov, lv_color_hex(0x000000), 0);
  lv_obj_set_style_bg_opa(ov, LV_OPA_80, 0);
  lv_obj_set_style_border_width(ov, 0, 0);
  lv_obj_set_style_radius(ov, 0, 0);
  lv_obj_set_style_pad_all(ov, 0, 0);
  lv_obj_clear_flag(ov, LV_OBJ_FLAG_SCROLLABLE);
  g_locOverlay = ov;

  // Panel de contenido (zona superior, sobre el teclado)
  lv_obj_t* panel = lv_obj_create(ov);
  lv_obj_set_size(panel, LV_HOR_RES, panH);
  lv_obj_align(panel, LV_ALIGN_TOP_LEFT, 0, 0);
  lv_obj_set_style_bg_color(panel, DV_PANEL, 0);
  lv_obj_set_style_bg_opa(panel, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(panel, 0, 0);
  lv_obj_set_style_radius(panel, 0, 0);
  lv_obj_set_style_pad_all(panel, 0, 0);
  lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLLABLE);

  // ── Título ──
  lv_obj_t* title = lv_label_create(panel);
  lv_label_set_text(title, LV_SYMBOL_GPS "  Ubicacion de medicion");
  lv_obj_set_style_text_color(title, DV_CYAN, 0);
  lv_obj_set_style_text_font(title, &lv_font_montserrat_14, 0);
  lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 5);

  // ── Textarea (campo de texto) ──
  lv_obj_t* ta = lv_textarea_create(panel);
  lv_textarea_set_one_line(ta, true);
  lv_textarea_set_max_length(ta, 31);
  lv_textarea_set_placeholder_text(ta, "Ej. Sala vege 1");
  lv_textarea_set_text(ta, g_uploadLocation.c_str());
  lv_obj_set_size(ta, 226, 34);
  lv_obj_align(ta, LV_ALIGN_TOP_MID, 0, 25);
  lv_obj_set_style_text_font(ta, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_font(ta, &lv_font_montserrat_14, LV_PART_CURSOR);
  g_locTextarea = ta;

  // ── [💾 Guardar] ──
  {
    lv_obj_t* b = lv_btn_create(panel);
    lv_obj_set_size(b, 110, 26);
    lv_obj_align(b, LV_ALIGN_TOP_LEFT, 5, 63);
    lv_obj_set_style_bg_color(b, DV_PANEL, 0);
    lv_obj_set_style_border_color(b, DV_CYAN, 0);
    lv_obj_set_style_border_width(b, 2, 0);
    lv_obj_add_event_cb(b, dv_locSave_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t* l = lv_label_create(b);
    lv_label_set_text(l, LV_SYMBOL_SAVE "  Guardar");
    lv_obj_set_style_text_font(l, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(l, DV_CYAN, 0);
    lv_obj_center(l);
  }

  // ── [🗑️ Borrar sel.] ──
  {
    lv_obj_t* b = lv_btn_create(panel);
    lv_obj_set_size(b, 110, 26);
    lv_obj_align(b, LV_ALIGN_TOP_RIGHT, -5, 63);
    lv_obj_set_style_bg_color(b, DV_PANEL, 0);
    lv_obj_set_style_border_color(b, lv_palette_main(LV_PALETTE_RED), 0);
    lv_obj_set_style_border_width(b, 2, 0);
    lv_obj_add_event_cb(b, dv_locDel_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t* l = lv_label_create(b);
    lv_label_set_text(l, LV_SYMBOL_TRASH "  Borrar sel.");
    lv_obj_set_style_text_font(l, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(l, lv_palette_main(LV_PALETTE_RED), 0);
    lv_obj_center(l);
  }

  // ── Grid 4×2 de nombres guardados ──
  for (int i = 0; i < DV_LOC_MAX; i++) {
    int col = i % 4, row = i / 4;
    lv_obj_t* b = lv_btn_create(panel);
    lv_obj_set_size(b, 54, 22);
    lv_obj_align(b, LV_ALIGN_TOP_LEFT, 5 + col * 58, 93 + row * 26);
    lv_obj_set_style_pad_all(b, 2, 0);
    bool sel = (i == g_locSelIdx);
    lv_obj_set_style_bg_color(b, sel ? DV_CYAN   : DV_PANEL, 0);
    lv_obj_set_style_bg_opa (b, sel ? LV_OPA_30 : LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(b, DV_CYAN, 0);
    lv_obj_set_style_border_width(b, 1, 0);
    lv_obj_add_event_cb(b, dv_locSel_cb, LV_EVENT_CLICKED, (void*)(intptr_t)i);
    lv_obj_t* l = lv_label_create(b);
    lv_label_set_long_mode(l, LV_LABEL_LONG_CLIP);
    lv_obj_set_width(l, 50);
    lv_obj_set_style_text_font(l, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(l, DV_CYAN, 0);
    lv_obj_center(l);
    if (i < g_locCount) {
      lv_label_set_text(l, g_locSaved[i]);
    } else {
      lv_label_set_text(l, "");
      lv_obj_add_flag(b, LV_OBJ_FLAG_HIDDEN);
    }
    g_locBtn[i] = b;
  }

  // ── [Cancelar] ──
  {
    lv_obj_t* b = lv_btn_create(panel);
    lv_obj_set_size(b, 110, 26);
    lv_obj_align(b, LV_ALIGN_TOP_LEFT, 5, 147);
    lv_obj_set_style_bg_color(b, DV_PANEL, 0);
    lv_obj_set_style_border_color(b, DV_GREY, 0);
    lv_obj_set_style_border_width(b, 2, 0);
    lv_obj_add_event_cb(b, dv_locCancel_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t* l = lv_label_create(b);
    lv_label_set_text(l, "Cancelar");
    lv_obj_set_style_text_font(l, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(l, DV_GREY, 0);
    lv_obj_center(l);
  }

  // ── [SUBIR ↑] ──
  {
    lv_obj_t* b = lv_btn_create(panel);
    lv_obj_set_size(b, 110, 26);
    lv_obj_align(b, LV_ALIGN_TOP_RIGHT, -5, 147);
    lv_obj_set_style_bg_color(b, DV_CYAN, 0);
    lv_obj_set_style_bg_opa(b, LV_OPA_30, 0);
    lv_obj_set_style_border_color(b, DV_CYAN, 0);
    lv_obj_set_style_border_width(b, 2, 0);
    lv_obj_add_event_cb(b, dv_locSend_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t* l = lv_label_create(b);
    lv_label_set_text(l, LV_SYMBOL_UPLOAD "  Subir");
    lv_obj_set_style_text_font(l, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(l, DV_CYAN, 0);
    lv_obj_center(l);
  }

  // ── Teclado LVGL (parte inferior) ──
  lv_obj_t* kb = lv_keyboard_create(ov);
  lv_obj_set_size(kb, LV_HOR_RES, kbH);
  lv_obj_align(kb, LV_ALIGN_BOTTOM_MID, 0, 0);
  lv_keyboard_set_textarea(kb, ta);
  lv_obj_set_style_text_font(kb, &lv_font_montserrat_14, 0);
}

// ───────────── Grafico de espectro (canvas, estilo full-spectrum) ─────────────
#define DV_SPEC_W          224       // ancho del canvas (px)
#define DV_SPEC_H          116       // alto del canvas (px)
#define DV_SPEC_FULL_PPFD  800.0f    // PPFD que llena el alto del grafico (ajustable)

// Perfil espectral de referencia (LED full-spectrum). x = nm, y = intensidad 0..1.
// Captura el pico azul ~450, el valle ~480, la joroba ancha ~590 y la cola roja.
static const float DV_SPEC_WL[] = {
  380, 410, 430, 445, 452, 460, 470, 482, 495, 515, 540, 565,
  585, 595, 610, 625, 640, 650, 662, 675, 695, 715, 740, 760, 780 };
static const float DV_SPEC_T[]  = {
  0.00f,0.08f,0.45f,0.85f,0.92f,0.85f,0.55f,0.27f,0.34f,0.50f,0.66f,0.82f,
  0.94f,1.00f,0.96f,0.86f,0.74f,0.64f,0.66f,0.58f,0.40f,0.24f,0.11f,0.04f,0.01f };
static const int DV_SPEC_NPTS = (int)(sizeof(DV_SPEC_WL) / sizeof(DV_SPEC_WL[0]));

// intensidad relativa del perfil en una longitud de onda dada (interp. lineal)
static float dv_specT(float wl) {
  if (wl <= DV_SPEC_WL[0])               return DV_SPEC_T[0];
  if (wl >= DV_SPEC_WL[DV_SPEC_NPTS - 1]) return DV_SPEC_T[DV_SPEC_NPTS - 1];
  for (int i = 1; i < DV_SPEC_NPTS; i++) {
    if (wl <= DV_SPEC_WL[i]) {
      float f = (wl - DV_SPEC_WL[i - 1]) / (DV_SPEC_WL[i] - DV_SPEC_WL[i - 1]);
      return DV_SPEC_T[i - 1] + f * (DV_SPEC_T[i] - DV_SPEC_T[i - 1]);
    }
  }
  return 0.0f;
}

// color aproximado de una longitud de onda visible (mapeo de Bruton)
static lv_color_t dv_wlColor(float wl) {
  float r = 0, g = 0, b = 0;
  if      (wl < 440) { r = -(wl - 440) / 60.0f; g = 0; b = 1; }
  else if (wl < 490) { r = 0; g = (wl - 440) / 50.0f; b = 1; }
  else if (wl < 510) { r = 0; g = 1; b = -(wl - 510) / 20.0f; }
  else if (wl < 580) { r = (wl - 510) / 70.0f; g = 1; b = 0; }
  else if (wl < 645) { r = 1; g = -(wl - 645) / 65.0f; b = 0; }
  else               { r = 1; g = 0; b = 0; }
  float f = 1.0f;                                  // atenuacion en los extremos
  if      (wl < 420) f = 0.3f + 0.7f * (wl - 380) / 40.0f;
  else if (wl > 700) f = 0.3f + 0.7f * (780 - wl) / 80.0f;
  if (f < 0) f = 0;
  int R = (int)(255.0f * powf(r < 0 ? 0 : r, 0.8f) * f);
  int G = (int)(255.0f * powf(g < 0 ? 0 : g, 0.8f) * f);
  int B = (int)(255.0f * powf(b < 0 ? 0 : b, 0.8f) * f);
  if (R > 255) R = 255; if (R < 0) R = 0;
  if (G > 255) G = 255; if (G < 0) G = 0;
  if (B > 255) B = 255; if (B < 0) B = 0;
  return lv_color_make((uint8_t)R, (uint8_t)G, (uint8_t)B);
}

// Ganancia de color en una longitud de onda: interpola entre los pesos R/G/B
// medidos, anclados a los picos del sensor (azul 460, verde 540, rojo 640 nm).
// Multiplica al perfil fijo => la curva se reforma segun el color, sin perder la silueta.
static float dv_specGain(float wl) {
  if (wl <= 460.0f) return g_specWB;
  if (wl <  540.0f) { float f = (wl - 460.0f) / 80.0f;  return g_specWB + f * (g_specWG - g_specWB); }
  if (wl <  640.0f) { float f = (wl - 540.0f) / 100.0f; return g_specWG + f * (g_specWR - g_specWG); }
  return g_specWR;
}

// Dibuja el espectro relleno en el canvas. amp = alto relativo (0..1).
// Escribe directo al buffer (set_px invalida por pixel -> seria lentisimo) e
// invalida una sola vez al final. Seguro: somos single-task (no corre durante el flush).
static void dv_drawSpectrum(float amp) {
  if (!g_specBuf || !g_spec) return;
  if (amp < 0.0f) amp = 0.0f;
  if (amp > 1.0f) amp = 1.0f;
  const int W = DV_SPEC_W, H = DV_SPEC_H;
  const lv_color_t bg = DV_BG;
  for (int x = 0; x < W; x++) {
    float wl = 380.0f + (float)x * (780.0f - 380.0f) / (float)(W - 1);
    float v  = dv_specT(wl) * dv_specGain(wl) * amp;
    if (v < 0) v = 0; if (v > 1) v = 1;
    int top = (H - 1) - (int)(v * (H - 1) + 0.5f);
    if (top < 0) top = 0; if (top > H - 1) top = H - 1;
    lv_color_t col = dv_wlColor(wl);
    for (int y = 0;   y < top; y++) g_specBuf[(uint32_t)y * W + x] = bg;   // fondo
    for (int y = top; y < H;   y++) g_specBuf[(uint32_t)y * W + x] = col;  // relleno
    g_specBuf[(uint32_t)top * W + x] = DV_WHITE;                           // contorno
  }
  lv_obj_invalidate(g_spec);
}

// Aplica la vista actual: grafico (def.) o controles (muestras/guardar/subir/borrar)
static void dv_applyMode() {
  bool graf = !g_showCtrls;
  dv_show(g_spec, graf);
  for (int i = 0; i < 3; i++) dv_show(g_specAx[i], graf);
  dv_show(g_lblPause, graf);
  dv_show(g_lblPct,      g_showCtrls);
  // FREE: sin nube ni multiples mediciones -> la vista de controles solo muestra RGB%.
  if (g_lblToggle)
    lv_label_set_text(g_lblToggle, g_showCtrls ? LV_SYMBOL_IMAGE : LV_SYMBOL_LIST);
}

static void dv_toggle_cb(lv_event_t* e) {
  static uint32_t lastToggle = 0;
  uint32_t now = millis();
  if (now - lastToggle < 400) return;    // debounce: el CST816D puede disparar 2 CLICKED por tap
  lastToggle = now;

  g_showCtrls = !g_showCtrls;
  dv_applyMode();
  if (!g_showCtrls) {                               // al volver al grafico, redibuja ya
    float amp = g_uiPpfd / DV_SPEC_FULL_PPFD;
    if (amp < 0.05f) amp = 0.05f;
    if (amp > 1.0f)  amp = 1.0f;
    dv_drawSpectrum(amp);
  }
}

// Redibuja el indicador de burbuja segun los angulos de inclinacion (grados).
// Canvas 26x26 px — centro en (13,13).
// roll  = inclinacion izquierda/derecha, pitch = inclinacion adelante/atras.
// Tolerancia nivelado: ±2°.
void uiSetLevel(float roll, float pitch) {
  if (!g_canvasLevel) return;

  bool ok = (fabsf(roll) < 2.0f && fabsf(pitch) < 2.0f);
  lv_color_t cBubble = ok ? lv_color_hex(0x00e060) : lv_color_hex(0xff3030);
  lv_color_t cBorder = ok ? lv_color_hex(0x00e060) : DV_GREY;

  lv_canvas_fill_bg(g_canvasLevel, DV_BG, LV_OPA_COVER);

  // borde exterior circular (24x24 dejando 1px de margen)
  lv_draw_rect_dsc_t bd;
  lv_draw_rect_dsc_init(&bd);
  bd.bg_opa       = LV_OPA_TRANSP;
  bd.border_color = cBorder;
  bd.border_width = 2;
  bd.border_opa   = LV_OPA_COVER;
  bd.radius       = LV_RADIUS_CIRCLE;
  lv_canvas_draw_rect(g_canvasLevel, 1, 1, 24, 24, &bd);

  // cruz de referencia
  lv_draw_line_dsc_t ld;
  lv_draw_line_dsc_init(&ld);
  ld.color = DV_GREY;
  ld.width = 1;
  ld.opa   = LV_OPA_40;
  static lv_point_t ln[2];
  ln[0] = {13, 3};  ln[1] = {13, 23};
  lv_canvas_draw_line(g_canvasLevel, ln, 2, &ld);
  ln[0] = {3, 13};  ln[1] = {23, 13};
  lv_canvas_draw_line(g_canvasLevel, ln, 2, &ld);

  // burbuja: 45° = desplazamiento maximo (8 px); radio burbuja = 3 px
  const float MAX_OFF = 8.0f;
  float dx = roll  / 45.0f * MAX_OFF;
  float dy = pitch / 45.0f * MAX_OFF;
  if (dx >  MAX_OFF) dx =  MAX_OFF;
  if (dx < -MAX_OFF) dx = -MAX_OFF;
  if (dy >  MAX_OFF) dy =  MAX_OFF;
  if (dy < -MAX_OFF) dy = -MAX_OFF;

  int bx = 13 + (int)roundf(dx);
  int by = 13 + (int)roundf(dy);

  lv_draw_rect_dsc_t fd;
  lv_draw_rect_dsc_init(&fd);
  fd.bg_color     = cBubble;
  fd.bg_opa       = LV_OPA_COVER;
  fd.border_width = 0;
  fd.radius       = LV_RADIUS_CIRCLE;
  lv_canvas_draw_rect(g_canvasLevel, bx - 3, by - 3, 6, 6, &fd);
}

static void dv_buildMedir(lv_obj_t* page) {
  dv_pageBase(page);

  // ── fila superior: toggle vista (izq) | nivel (centro) | S | nube | bat | wifi (der) ──
  // toggle: alterna grafico (def.) <-> controles (muestras/guardar/subir/borrar)
  g_btnToggle = dv_borderBtn(page, 44, 26, LV_ALIGN_TOP_LEFT, 6, 4, dv_toggle_cb);
  lv_obj_set_ext_click_area(g_btnToggle, 16);   // mismo tamaño visual, pero +16px de "colchon" alrededor para tocarlo mas facil
  g_lblToggle = lv_label_create(g_btnToggle);
  lv_label_set_text(g_lblToggle, LV_SYMBOL_LIST);
  lv_obj_set_style_text_color(g_lblToggle, DV_CYAN, 0);
  lv_obj_center(g_lblToggle);

  // estado arriba a la derecha (de der a izq): WiFi | bateria | nube | S(sensor)
  g_lblWifi = lv_label_create(page);
  lv_label_set_text(g_lblWifi, LV_SYMBOL_WIFI);
  lv_obj_set_style_text_color(g_lblWifi, DV_GREY, 0);
  lv_obj_align(g_lblWifi, LV_ALIGN_TOP_RIGHT, -8, 6);

  g_lblBatt = lv_label_create(page);
  lv_label_set_text(g_lblBatt, LV_SYMBOL_BATTERY_FULL);
  lv_obj_set_style_text_color(g_lblBatt, DV_GREY, 0);
  lv_obj_align(g_lblBatt, LV_ALIGN_TOP_RIGHT, -30, 6);

  // linea diagonal sobre la bateria (solo visible si no hay bateria => "tachado")
  g_battSlashPts[0].x = 0;  g_battSlashPts[0].y = 13;
  g_battSlashPts[1].x = 15; g_battSlashPts[1].y = 0;
  g_battSlash = lv_line_create(page);
  lv_line_set_points(g_battSlash, g_battSlashPts, 2);
  lv_obj_set_style_line_color(g_battSlash, lv_palette_main(LV_PALETTE_RED), 0);
  lv_obj_set_style_line_width(g_battSlash, 2, 0);
  lv_obj_align_to(g_battSlash, g_lblBatt, LV_ALIGN_CENTER, 0, 0);
  lv_obj_add_flag(g_battSlash, LV_OBJ_FLAG_HIDDEN);

  // FREE: sin icono de estado de nube (no hay subida).

  // "S" roja => sensor TCS no detectado (oculta si el sensor responde)
  g_lblSensor = lv_label_create(page);
  lv_label_set_text(g_lblSensor, "S");
  lv_obj_set_style_text_color(g_lblSensor, lv_palette_main(LV_PALETTE_RED), 0);
  lv_obj_align(g_lblSensor, LV_ALIGN_TOP_RIGHT, -74, 6);
  if (g_tcsOK) lv_obj_add_flag(g_lblSensor, LV_OBJ_FLAG_HIDDEN);

  // ── Indicador de nivel (burbuja 26×26 px, en lugar del voltaje de bateria) ──
  g_canvasLevel = lv_canvas_create(page);
  lv_canvas_set_buffer(g_canvasLevel, g_levelBuf, 26, 26, LV_IMG_CF_TRUE_COLOR);
  lv_obj_align(g_canvasLevel, LV_ALIGN_TOP_MID, 0, 2);
  uiSetLevel(0.0f, 0.0f);   // dibujo inicial

  // ── PPFD (SIEMPRE visible, en grande) ──
  lv_obj_t* t = lv_label_create(page);
  lv_label_set_text(t, "PPFD");
  lv_obj_set_style_text_color(t, DV_CYAN, 0);
  lv_obj_align(t, LV_ALIGN_TOP_MID, 0, 32);

  g_lblPpfd = lv_label_create(page);
  lv_obj_set_style_text_font(g_lblPpfd, DV_PPFD_FONT, 0);
  lv_obj_set_style_text_color(g_lblPpfd, DV_WHITE, 0);
  lv_label_set_text(g_lblPpfd, "0");
  lv_obj_align(g_lblPpfd, LV_ALIGN_TOP_MID, 0, 50);
  lv_obj_add_flag(g_lblPpfd, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(g_lblPpfd, dv_pause_cb, LV_EVENT_ALL, NULL);

  lv_obj_t* u = lv_label_create(page);
  lv_label_set_text(u, "umol/m2.s");
  lv_obj_set_style_text_color(u, DV_CYAN2, 0);
  lv_obj_align(u, LV_ALIGN_TOP_MID, 0, 102);

  // ── GRAFICO de espectro (canvas) — vista por defecto ──
  g_specBuf = (lv_color_t*)heap_caps_malloc((size_t)DV_SPEC_W * DV_SPEC_H * sizeof(lv_color_t),
                                            MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  if (!g_specBuf)
    g_specBuf = (lv_color_t*)heap_caps_malloc((size_t)DV_SPEC_W * DV_SPEC_H * sizeof(lv_color_t),
                                              MALLOC_CAP_8BIT);
  g_spec = lv_canvas_create(page);
  if (g_specBuf) lv_canvas_set_buffer(g_spec, g_specBuf, DV_SPEC_W, DV_SPEC_H, LV_IMG_CF_TRUE_COLOR);
  lv_obj_align(g_spec, LV_ALIGN_TOP_MID, 0, 122);
  lv_obj_add_flag(g_spec, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(g_spec, dv_pause_cb, LV_EVENT_ALL, NULL);
  dv_drawSpectrum(0.06f);                              // silueta tenue inicial

  // etiquetas de eje (longitud de onda en nm)
  static const char* axTxt[3] = { "380", "580", "780nm" };
  static const int   axX[3]   = { -100, 0, 96 };
  for (int i = 0; i < 3; i++) {
    g_specAx[i] = lv_label_create(page);
    lv_label_set_text(g_specAx[i], axTxt[i]);
    lv_obj_set_style_text_color(g_specAx[i], DV_GREY, 0);
    lv_obj_align(g_specAx[i], LV_ALIGN_TOP_MID, axX[i], 240);
  }

  // Play/Pausa: entre el grafico y las pestañas — indica si la lectura esta corriendo o congelada
  g_lblPause = lv_label_create(page);
  lv_label_set_text(g_lblPause, g_paused ? LV_SYMBOL_PAUSE : LV_SYMBOL_PLAY);
  lv_obj_set_style_text_color(g_lblPause, DV_GREY, 0);
  lv_obj_align(g_lblPause, LV_ALIGN_TOP_MID, 0, 260);

  // ── CONTROLES (vista alterna: ocultos por defecto) ──
  g_lblPct = lv_label_create(page);
  lv_label_set_text(g_lblPct, "R --   G --   B --");
  lv_obj_set_style_text_color(g_lblPct, DV_CYAN2, 0);
  lv_obj_align(g_lblPct, LV_ALIGN_TOP_MID, 0, 118);

  // FREE: sin estado de nube ni botones Guardar/Subir/Borrar (multiples mediciones
  // + subida a la nube eliminados). La vista de controles queda como detalle RGB%.

  dv_applyMode();        // estado inicial: grafico visible, controles ocultos
}

// ───────────────────────── DLI ─────────────────────────
static void dv_recalcDLI() {
  if (!g_lblDli) return;
  float dli = g_uiPpfd * 3600.0f * (float)g_dliHours / 1e6f;   // mol/m2.dia
  char b[24];
  snprintf(b, sizeof(b), "%.1f", dli);
  lv_label_set_text(g_lblDli, b);
  lv_label_set_text_fmt(g_lblDliInfo, "%d umol  x  %d h", (int)lroundf(g_uiPpfd), g_dliHours);
}

static void dv_roller_cb(lv_event_t* e) {
  g_dliHours = (int)lv_roller_get_selected(g_rollerHours);
  dv_recalcDLI();
}

static void dv_buildDLI(lv_obj_t* page) {
  dv_pageBase(page);

  lv_obj_t* t = lv_label_create(page);
  lv_label_set_text(t, "DLI");
  lv_obj_set_style_text_color(t, DV_CYAN, 0);
  lv_obj_align(t, LV_ALIGN_TOP_MID, 0, 8);

  lv_obj_t* h = lv_label_create(page);
  lv_label_set_text(h, "Horas de luz");
  lv_obj_set_style_text_color(h, DV_CYAN2, 0);
  lv_obj_align(h, LV_ALIGN_TOP_LEFT, 16, 44);

  g_rollerHours = lv_roller_create(page);
  lv_roller_set_options(g_rollerHours,
    "0\n1\n2\n3\n4\n5\n6\n7\n8\n9\n10\n11\n12\n13\n14\n15\n16\n17\n18\n19\n20\n21\n22\n23\n24",
    LV_ROLLER_MODE_NORMAL);
  lv_roller_set_visible_row_count(g_rollerHours, 3);
  lv_obj_set_width(g_rollerHours, 72);
  lv_obj_align(g_rollerHours, LV_ALIGN_TOP_LEFT, 16, 70);
  lv_roller_set_selected(g_rollerHours, g_dliHours, LV_ANIM_OFF);
  lv_obj_set_style_bg_color(g_rollerHours, DV_PANEL, 0);
  lv_obj_set_style_text_color(g_rollerHours, DV_WHITE, 0);
  lv_obj_set_style_bg_color(g_rollerHours, DV_CYAN, LV_PART_SELECTED);
  lv_obj_set_style_text_color(g_rollerHours, lv_color_hex(0x001018), LV_PART_SELECTED);
  lv_obj_add_event_cb(g_rollerHours, dv_roller_cb, LV_EVENT_VALUE_CHANGED, NULL);

  g_lblDli = lv_label_create(page);
  lv_obj_set_style_text_font(g_lblDli, DV_MED_FONT, 0);
  lv_obj_set_style_text_color(g_lblDli, DV_WHITE, 0);
  lv_label_set_text(g_lblDli, "0.0");
  lv_obj_align(g_lblDli, LV_ALIGN_TOP_RIGHT, -24, 86);

  lv_obj_t* un = lv_label_create(page);
  lv_label_set_text(un, "mol/m2.dia");
  lv_obj_set_style_text_color(un, DV_CYAN2, 0);
  lv_obj_align(un, LV_ALIGN_TOP_RIGHT, -24, 128);

  g_lblDliInfo = lv_label_create(page);
  lv_label_set_text(g_lblDliInfo, "-");
  lv_obj_set_style_text_color(g_lblDliInfo, DV_GREY, 0);
  lv_obj_align(g_lblDliInfo, LV_ALIGN_BOTTOM_MID, 0, -16);
}

// ───────────────────────── Calibracion ─────────────────────────
static void dv_updateCalLabels() {
  char b[24];
  snprintf(b, sizeof(b), "%.3f", g_factor); lv_label_set_text(g_lblFactor, b);
  snprintf(b, sizeof(b), "%.1f", g_offset); lv_label_set_text(g_lblOffset, b);
}

static void dv_cal_cb(lv_event_t* e) {
  intptr_t which = (intptr_t)lv_event_get_user_data(e);
  switch (which) {
    case 0: g_factor -= 0.05f; break;
    case 1: g_factor += 0.05f; break;
    case 2: g_offset -= 10.0f; break;
    case 3: g_offset += 10.0f; break;
  }
  if (g_factor < 0.0f) g_factor = 0.0f;
  storeSaveCalCustom();          // ajuste manual (+/-) -> calib 'custom' (el OTA no la pisa)
  dv_updateCalLabels();
}

// ───────────── Menu "Paneles LED" (elegir marca/modelo -> calibracion) ─────────────
// arma la lista de marcas unicas (en orden de aparicion en g_panels), separadas por '\n'
static String dv_panelBrandOptions() {
  String out;
  for (auto& p : g_panels) {
    bool found = false;
    int from = 0;
    while (true) {
      int nl = out.indexOf('\n', from);
      String b = (nl < 0) ? out.substring(from) : out.substring(from, nl);
      if (b == p.brand) { found = true; break; }
      if (nl < 0) break;
      from = nl + 1;
    }
    if (!found) { if (out.length()) out += '\n'; out += p.brand; }
  }
  return out.length() ? out : String("(sin datos)");
}

// arma la lista de modelos de una marca dada, separados por '\n'
static String dv_panelModelOptions(const String& brand) {
  String out;
  for (auto& p : g_panels) {
    if (p.brand != brand) continue;
    if (out.length()) out += '\n';
    out += p.model;
  }
  return out.length() ? out : String("(sin modelos)");
}

// repuebla el desplegable de Modelo segun la marca elegida (selecciona el primero)
static void dv_panelRefreshModels() {
  if (!g_ddBrand || !g_ddModel) return;
  char brand[40];
  lv_dropdown_get_selected_str(g_ddBrand, brand, sizeof(brand));
  lv_dropdown_set_options(g_ddModel, dv_panelModelOptions(String(brand)).c_str());
  lv_dropdown_set_selected(g_ddModel, 0);
}

// reconstruye ambos desplegables desde g_panels (al abrir el menu o tras "Actualizar lista")
static void dv_panelRefreshAll() {
  if (!g_ddBrand) return;
  lv_dropdown_set_options(g_ddBrand, dv_panelBrandOptions().c_str());
  lv_dropdown_set_selected(g_ddBrand, 0);
  dv_panelRefreshModels();
}

static void dv_ddBrand_cb(lv_event_t* e) {
  dv_panelRefreshModels();
}

static void dv_panelMenuOpen_cb(lv_event_t* e) {
  if (!g_panelMenu) return;
  dv_panelRefreshAll();
  if (g_lblPanelMsg) {
    lv_obj_set_style_text_color(g_lblPanelMsg, DV_GREY, 0);
    lv_label_set_text(g_lblPanelMsg, g_panels.empty() ? "Lista no disponible"
                                                       : "Elegi marca y modelo");
  }
  lv_obj_clear_flag(g_panelMenu, LV_OBJ_FLAG_HIDDEN);
}

static void dv_panelMenuClose_cb(lv_event_t* e) {
  if (g_panelMenu) lv_obj_add_flag(g_panelMenu, LV_OBJ_FLAG_HIDDEN);
}

// el menu "Paneles LED" es un overlay aparte del tabview: si el usuario cambia
// de pestaña (Medir, DLI, etc.) con el menu abierto, hay que cerrarlo solo
static void dv_tabChange_cb(lv_event_t* e) {
  if (g_panelMenu && !lv_obj_has_flag(g_panelMenu, LV_OBJ_FLAG_HIDDEN))
    lv_obj_add_flag(g_panelMenu, LV_OBJ_FLAG_HIDDEN);
}

// "Aplicar": copia el factor/offset del panel elegido como calibracion activa y persiste
static void dv_panelApply_cb(lv_event_t* e) {
  if (g_panels.empty() || !g_ddBrand || !g_ddModel) return;
  char brand[40], model[40];
  lv_dropdown_get_selected_str(g_ddBrand, brand, sizeof(brand));
  lv_dropdown_get_selected_str(g_ddModel, model, sizeof(model));

  for (auto& p : g_panels) {
    if (p.brand == brand && p.model == model) {
      g_factor = p.factor;
      g_offset = p.offset;
      storeSaveCalPanel(String(brand), String(model));   // guarda el vinculo: el OTA auto-actualiza este panel
      dv_updateCalLabels();
      if (g_lblPanelMsg) {
        char b[64];
        snprintf(b, sizeof(b), "Aplicado: %s %s", brand, model);
        lv_obj_set_style_text_color(g_lblPanelMsg, lv_palette_main(LV_PALETTE_GREEN), 0);
        lv_label_set_text(g_lblPanelMsg, b);
      }
      return;
    }
  }
  if (g_lblPanelMsg) {
    lv_obj_set_style_text_color(g_lblPanelMsg, lv_palette_main(LV_PALETTE_RED), 0);
    lv_label_set_text(g_lblPanelMsg, "No encontrado");
  }
}

// arma el overlay del menu (oculto hasta que se abra desde el boton "Paneles LED" en Calib)
static void dv_buildPanelMenu(lv_obj_t* scr) {
  g_panelMenu = lv_obj_create(scr);
  lv_obj_set_size(g_panelMenu, 224, 254);
  lv_obj_align(g_panelMenu, LV_ALIGN_CENTER, 0, -10);
  lv_obj_set_style_bg_color(g_panelMenu, DV_PANEL, 0);
  lv_obj_set_style_border_color(g_panelMenu, DV_CYAN, 0);
  lv_obj_set_style_border_width(g_panelMenu, 2, 0);
  lv_obj_set_style_radius(g_panelMenu, 10, 0);
  lv_obj_clear_flag(g_panelMenu, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(g_panelMenu, LV_OBJ_FLAG_HIDDEN);

  lv_obj_t* t = lv_label_create(g_panelMenu);
  lv_label_set_text(t, "Paneles LED");
  lv_obj_set_style_text_color(t, DV_CYAN, 0);
  lv_obj_align(t, LV_ALIGN_TOP_MID, 0, 8);

  // cerrar (X) arriba a la derecha
  lv_obj_t* bx = dv_borderBtn(g_panelMenu, 30, 26, LV_ALIGN_TOP_RIGHT, -6, 6, dv_panelMenuClose_cb);
  lv_obj_t* lx = lv_label_create(bx);
  lv_label_set_text(lx, LV_SYMBOL_CLOSE);
  lv_obj_set_style_text_color(lx, DV_CYAN2, 0);
  lv_obj_center(lx);

  lv_obj_t* lm = lv_label_create(g_panelMenu);
  lv_label_set_text(lm, "Marca");
  lv_obj_set_style_text_color(lm, DV_CYAN2, 0);
  lv_obj_align(lm, LV_ALIGN_TOP_LEFT, 12, 44);
  g_ddBrand = lv_dropdown_create(g_panelMenu);
  lv_dropdown_set_options(g_ddBrand, "(sin datos)");
  lv_obj_set_width(g_ddBrand, 200);
  lv_obj_align(g_ddBrand, LV_ALIGN_TOP_MID, 0, 64);
  lv_obj_add_event_cb(g_ddBrand, dv_ddBrand_cb, LV_EVENT_VALUE_CHANGED, NULL);

  lv_obj_t* lmo = lv_label_create(g_panelMenu);
  lv_label_set_text(lmo, "Modelo");
  lv_obj_set_style_text_color(lmo, DV_CYAN2, 0);
  lv_obj_align(lmo, LV_ALIGN_TOP_LEFT, 12, 104);
  g_ddModel = lv_dropdown_create(g_panelMenu);
  lv_dropdown_set_options(g_ddModel, "(sin datos)");
  lv_obj_set_width(g_ddModel, 200);
  lv_obj_align(g_ddModel, LV_ALIGN_TOP_MID, 0, 124);

  // Aplicar
  lv_obj_t* ba = lv_btn_create(g_panelMenu);
  lv_obj_set_size(ba, 200, 36);
  lv_obj_align(ba, LV_ALIGN_TOP_MID, 0, 168);
  lv_obj_set_style_bg_color(ba, DV_PANEL, 0);
  lv_obj_set_style_border_color(ba, DV_CYAN, 0);
  lv_obj_set_style_border_width(ba, 2, 0);
  lv_obj_add_event_cb(ba, dv_panelApply_cb, LV_EVENT_CLICKED, NULL);
  lv_obj_t* lba = lv_label_create(ba);
  lv_label_set_text(lba, "Aplicar calibracion");
  lv_obj_set_style_text_color(lba, DV_CYAN, 0);
  lv_obj_center(lba);

  // estado / resultado (se reusa tambien para el progreso de "Actualizar lista")
  g_lblPanelMsg = lv_label_create(g_panelMenu);
  lv_label_set_long_mode(g_lblPanelMsg, LV_LABEL_LONG_WRAP);
  lv_obj_set_width(g_lblPanelMsg, 204);
  lv_obj_set_style_text_align(g_lblPanelMsg, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_color(g_lblPanelMsg, DV_GREY, 0);
  lv_label_set_text(g_lblPanelMsg, "");
  lv_obj_align(g_lblPanelMsg, LV_ALIGN_TOP_MID, 0, 212);
}

// (Removido: "Actualizar lista" por red. La lista de paneles vive embebida en el
//  firmware y se actualiza por OTA — ver panels.h.)

// FREE: "Actualizar por USB" — reinicia el equipo a MODO DESCARGA de ROM para flashear
// por USB sin boton BOOT (el producto no lo tiene). El loop atiende g_dlModeRequested.
static void dv_dlConfirm_cb(lv_event_t* e) {
  lv_obj_t* mb = lv_event_get_current_target(e);
  const char* txt = lv_msgbox_get_active_btn_text(mb);
  if (txt && strcmp(txt, "Si, actualizar") == 0) g_dlModeRequested = true;
  lv_msgbox_close(mb);
}
static void dv_updateUsb_cb(lv_event_t* e) {
  static const char* btns[] = { "Si, actualizar", "Cancelar", "" };
  lv_obj_t* mb = lv_msgbox_create(NULL, "Actualizar por USB",
    "El equipo se reiniciara en MODO USB (pantalla apagada) para conectarse a la PC y "
    "flashear el firmware.\n\nPara salir sin actualizar: desenchufa y enchufa de nuevo.",
    btns, false);
  lv_obj_center(mb);
  lv_obj_add_event_cb(mb, dv_dlConfirm_cb, LV_EVENT_VALUE_CHANGED, NULL);
}

static void dv_buildCalib(lv_obj_t* page) {
  dv_pageBase(page);

  lv_obj_t* t = lv_label_create(page);
  lv_label_set_text(t, "Calibracion");
  lv_obj_set_style_text_color(t, DV_CYAN, 0);
  lv_obj_align(t, LV_ALIGN_TOP_MID, 0, 8);

  // Factor
  lv_obj_t* lf = lv_label_create(page);
  lv_label_set_text(lf, "Factor");
  lv_obj_set_style_text_color(lf, DV_CYAN2, 0);
  lv_obj_align(lf, LV_ALIGN_TOP_LEFT, 20, 46);
  dv_stepBtn(page, "-", LV_ALIGN_TOP_LEFT,  20, 66, dv_cal_cb, 0);
  g_lblFactor = lv_label_create(page);
  lv_obj_set_style_text_font(g_lblFactor, DV_MED_FONT, 0);
  lv_obj_set_style_text_color(g_lblFactor, DV_WHITE, 0);
  lv_obj_align(g_lblFactor, LV_ALIGN_TOP_MID, 0, 72);
  dv_stepBtn(page, "+", LV_ALIGN_TOP_RIGHT, -20, 66, dv_cal_cb, 1);

  // Offset
  lv_obj_t* lo = lv_label_create(page);
  lv_label_set_text(lo, "Offset");
  lv_obj_set_style_text_color(lo, DV_CYAN2, 0);
  lv_obj_align(lo, LV_ALIGN_TOP_LEFT, 20, 120);
  dv_stepBtn(page, "-", LV_ALIGN_TOP_LEFT,  20, 140, dv_cal_cb, 2);
  g_lblOffset = lv_label_create(page);
  lv_obj_set_style_text_font(g_lblOffset, DV_MED_FONT, 0);
  lv_obj_set_style_text_color(g_lblOffset, DV_WHITE, 0);
  lv_obj_align(g_lblOffset, LV_ALIGN_TOP_MID, 0, 146);
  dv_stepBtn(page, "+", LV_ALIGN_TOP_RIGHT, -20, 140, dv_cal_cb, 3);

  lv_obj_t* hint = lv_label_create(page);
  lv_label_set_text(hint, "PPFD = cubica * factor + offset");
  lv_obj_set_style_text_color(hint, DV_GREY, 0);
  lv_obj_align(hint, LV_ALIGN_TOP_MID, 0, 196);

  // Paneles LED: elegir marca/modelo -> aplica el factor/offset de ese panel
  lv_obj_t* bp = lv_btn_create(page);
  lv_obj_set_size(bp, 220, 44);
  lv_obj_align(bp, LV_ALIGN_BOTTOM_MID, 0, -56);
  lv_obj_set_style_bg_color(bp, DV_PANEL, 0);
  lv_obj_set_style_border_color(bp, DV_CYAN2, 0);
  lv_obj_set_style_border_width(bp, 2, 0);
  lv_obj_add_event_cb(bp, dv_panelMenuOpen_cb, LV_EVENT_CLICKED, NULL);
  lv_obj_t* lbp = lv_label_create(bp);
  lv_label_set_text(lbp, LV_SYMBOL_LIST "  Paneles LED");
  lv_obj_set_style_text_color(lbp, DV_CYAN2, 0);
  lv_obj_center(lbp);

  // FREE: Actualizar firmware por USB (reemplaza al boton BOOT en equipos sin boton)
  lv_obj_t* bu = lv_btn_create(page);
  lv_obj_set_size(bu, 220, 40);
  lv_obj_align(bu, LV_ALIGN_BOTTOM_MID, 0, -8);
  lv_obj_set_style_bg_color(bu, DV_PANEL, 0);
  lv_obj_set_style_border_color(bu, DV_GREY, 0);
  lv_obj_set_style_border_width(bu, 2, 0);
  lv_obj_add_event_cb(bu, dv_updateUsb_cb, LV_EVENT_CLICKED, NULL);
  lv_obj_t* lbu = lv_label_create(bu);
  lv_label_set_text(lbu, LV_SYMBOL_DOWNLOAD "  Actualizar por USB");
  lv_obj_set_style_text_color(lbu, DV_GREY, 0);
  lv_obj_center(lbu);

  dv_updateCalLabels();
}

// ───────────────────────── Config / WiFi ─────────────────────────

// ── Panel de ingreso WiFi en horizontal ──────────────────────────
// Al tocar un campo en Config (IP / SSID / Clave) se gira el display a horizontal y se
// abre este overlay con UN solo campo (el que se esta editando) grande arriba + teclado
// ancho abajo. Al confirmar (✓) se vuelca el valor al campo de Config y se vuelve a vertical.

// Ojo del overlay: ver/ocultar la clave (sincroniza con el ojo de Config via g_passShown).
static void dv_wifiKbEye_cb(lv_event_t* e) {
  g_passShown = !g_passShown;
  if (g_wifiKbField) lv_textarea_set_password_mode(g_wifiKbField, !g_passShown);
  if (g_wifiKbEye)   lv_label_set_text(g_wifiKbEye, g_passShown ? LV_SYMBOL_EYE_CLOSE
                                                               : LV_SYMBOL_EYE_OPEN);
}

// Cierra el overlay: vuelca el valor al campo de Config y vuelve a vertical.
static void dv_closeWifiKb() {
  if (!g_wifiKbScr) return;
  if (g_wifiKbTarget) lv_textarea_set_text(g_wifiKbTarget, lv_textarea_get_text(g_wifiKbField));
  if (g_wifiKbTarget == g_taPass) {                          // refleja el ojo en Config
    lv_textarea_set_password_mode(g_taPass, !g_passShown);
    if (g_lblEye) lv_label_set_text(g_lblEye, g_passShown ? LV_SYMBOL_EYE_CLOSE
                                                          : LV_SYMBOL_EYE_OPEN);
  }
  lv_obj_del(g_wifiKbScr);
  g_wifiKbScr = g_wifiKbField = g_wifiKbEye = g_wifiKbKbd = g_wifiKbTarget = nullptr;
  displaySetRotation(false);                                 // vuelve a vertical
  lv_obj_invalidate(lv_scr_act());
}

// ✓ / ✗ del teclado -> cerrar (en ambos casos conservamos lo tipeado).
static void dv_wifiKbKbd_cb(lv_event_t* e) {
  lv_event_code_t c = lv_event_get_code(e);
  if (c == LV_EVENT_READY || c == LV_EVENT_CANCEL) dv_closeWifiKb();
}

// Abre el overlay horizontal para editar UN campo (focus = el textarea de Config tocado).
static void dv_openWifiKb(lv_obj_t* focus) {
  if (g_wifiKbScr) return;
  g_wifiKbTarget = focus;
  bool isPass = (focus == g_taPass);
  bool isIp   = (focus == g_taIp);
  const char* caption = isPass ? "Clave WiFi"
                      : isIp   ? "IP del bot"
                               : "Red WiFi (SSID)";

  displaySetRotation(true);                                  // horizontal: 320x240

  lv_obj_t* ov = lv_obj_create(lv_scr_act());
  lv_obj_set_size(ov, 320, 240);
  lv_obj_set_pos(ov, 0, 0);
  lv_obj_set_style_bg_color(ov, DV_BG, 0);
  lv_obj_set_style_bg_opa(ov, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(ov, 0, 0);
  lv_obj_set_style_radius(ov, 0, 0);
  lv_obj_set_style_pad_all(ov, 0, 0);
  lv_obj_clear_flag(ov, LV_OBJ_FLAG_SCROLLABLE);
  g_wifiKbScr = ov;

  // ── Titulo del campo ──
  lv_obj_t* cap = lv_label_create(ov);
  lv_label_set_text(cap, caption);
  lv_obj_set_style_text_color(cap, DV_CYAN2, 0);
  lv_obj_set_pos(cap, 10, 8);

  // ── Campo unico (lo que se tipea, en grande) ──
  lv_obj_t* ta = lv_textarea_create(ov);
  lv_textarea_set_one_line(ta, true);
  lv_textarea_set_password_mode(ta, isPass && !g_passShown);
  lv_textarea_set_text(ta, lv_textarea_get_text(focus));
  lv_obj_set_size(ta, isPass ? 262 : 304, 44);
  lv_obj_set_pos(ta, 8, 30);
  lv_obj_set_style_text_font(ta, DV_MED_FONT, 0);
  g_wifiKbField = ta;

  if (isPass) {                                              // ojo solo para la clave
    lv_obj_t* be = dv_borderBtn(ov, 38, 44, LV_ALIGN_TOP_LEFT, 274, 30, dv_wifiKbEye_cb);
    g_wifiKbEye = lv_label_create(be);
    lv_label_set_text(g_wifiKbEye, g_passShown ? LV_SYMBOL_EYE_CLOSE : LV_SYMBOL_EYE_OPEN);
    lv_obj_set_style_text_color(g_wifiKbEye, DV_CYAN, 0);
    lv_obj_center(g_wifiKbEye);
  }

  // ── Teclado ancho (mitad inferior) ──
  lv_obj_t* kb = lv_keyboard_create(ov);
  lv_obj_set_size(kb, 320, 150);
  lv_obj_align(kb, LV_ALIGN_BOTTOM_MID, 0, 0);
  lv_obj_set_style_text_font(kb, &lv_font_montserrat_14, 0);
  lv_keyboard_set_mode(kb, isIp ? LV_KEYBOARD_MODE_NUMBER : LV_KEYBOARD_MODE_TEXT_LOWER);
  lv_keyboard_set_textarea(kb, ta);
  lv_obj_add_event_cb(kb, dv_wifiKbKbd_cb, LV_EVENT_ALL, NULL);
  g_wifiKbKbd = kb;
}

static void dv_kb_cb(lv_event_t* e) {
  lv_event_code_t c = lv_event_get_code(e);
  if (c == LV_EVENT_READY || c == LV_EVENT_CANCEL)
    lv_obj_add_flag(g_kb, LV_OBJ_FLAG_HIDDEN);
}

static void dv_ta_cb(lv_event_t* e) {
  lv_event_code_t c  = lv_event_get_code(e);
  lv_obj_t*       ta = lv_event_get_target(e);
  if (c == LV_EVENT_FOCUSED || c == LV_EVENT_CLICKED)      // CLICKED: confiable en touch
    dv_openWifiKb(ta);   // abre el teclado ancho en horizontal (campos arriba)
}

void uiSetWifi(bool connected);         // definido mas abajo (lo usan dv_connect_cb/dv_scan_cb/el switch WiFi)

static void dv_connect_cb(lv_event_t* e) {
  g_ssid  = lv_textarea_get_text(g_taSsid);
  g_pass  = lv_textarea_get_text(g_taPass);
  g_botIp = lv_textarea_get_text(g_taIp);   // se guarda por comodidad; emparejar usa este valor
  g_botIp.trim();
  storeSaveNet();
  if (!g_wifiEnabled) {              // radio apagada (por defecto, ahorro): prenderla para conectar
    g_wifiEnabled = true;
    storeSaveFlags();
    netSetEnabled(true);
    uiSetWifi(g_wifiConnected);
  }
  netApply();
  if (g_lblNetStatus) lv_label_set_text(g_lblNetStatus, "Conectando...");
  lv_obj_add_flag(g_kb, LV_OBJ_FLAG_HIDDEN);
}

// Switch de emparejamiento: ON = intentar emparejar, OFF = limpiar device_id
static void dv_swPair_cb(lv_event_t* e) {
  bool on = lv_obj_has_state(lv_event_get_target(e), LV_STATE_CHECKED);
  if (on) {
    g_botIp = lv_textarea_get_text(g_taIp);
    g_botIp.trim();
    g_pairPending = true;
    if (g_lblNetStatus) {
      lv_obj_set_style_text_color(g_lblNetStatus, DV_CYAN2, 0);
      lv_label_set_text(g_lblNetStatus, "Emparejando...");
    }
    lv_obj_add_flag(g_kb, LV_OBJ_FLAG_HIDDEN);
  } else {
    g_deviceId = "";
    storeSaveNet();
    if (g_lblNetStatus) {
      lv_obj_set_style_text_color(g_lblNetStatus, DV_GREY, 0);
      lv_label_set_text(g_lblNetStatus, "Desemparejado");
    }
  }
}

// Buscar redes: si la radio esta apagada (por defecto, ahorro) hay que prenderla
// primero o el scan no encuentra nada ("(sin redes)"). El switch WiFi se sincroniza
// solo si esto termina en una conexion (uiSetWifi via netLoop).
static void dv_scan_cb(lv_event_t* e) {
  if (!g_wifiEnabled) {
    g_wifiEnabled = true;
    storeSaveFlags();
    netSetEnabled(true);
    uiSetWifi(g_wifiConnected);
  }
  netScanStart();
  lv_dropdown_set_options(g_ddNetworks, "Buscando...");
}

static void dv_dd_cb(lv_event_t* e) {
  char buf[40];
  lv_dropdown_get_selected_str(g_ddNetworks, buf, sizeof(buf));
  if (strcmp(buf, "Buscando...") && strcmp(buf, "(Buscar redes)") && strcmp(buf, "(sin redes)"))
    lv_textarea_set_text(g_taSsid, buf);
}

static void dv_eye_cb(lv_event_t* e) {
  g_passShown = !g_passShown;
  lv_textarea_set_password_mode(g_taPass, !g_passShown);
  lv_label_set_text(g_lblEye, g_passShown ? LV_SYMBOL_EYE_CLOSE : LV_SYMBOL_EYE_OPEN);
}

static lv_obj_t* dv_makeTa(lv_obj_t* page, const char* ph, int y,
                           const char* val, int w, bool pass) {
  lv_obj_t* ta = lv_textarea_create(page);
  lv_textarea_set_one_line(ta, true);
  lv_textarea_set_password_mode(ta, pass);
  lv_textarea_set_placeholder_text(ta, ph);
  lv_textarea_set_text(ta, val);
  lv_obj_set_width(ta, w);
  lv_obj_align(ta, LV_ALIGN_TOP_LEFT, 12, y);
  lv_obj_add_event_cb(ta, dv_ta_cb, LV_EVENT_ALL, NULL);
  return ta;
}

static void dv_ota_cb(lv_event_t* e);   // OTA: definido en la seccion de abajo

// Switch WiFi: representa "conectado a una red con internet". ON = prender la
// radio e intentar conectar (si falla o se cae despues, netLoop lo apaga solo
// -> ver uiSetWifi). OFF = apagar la radio (ahorra bateria).
static void dv_wifiSw_cb(lv_event_t* e) {
  bool on = lv_obj_has_state(lv_event_get_target(e), LV_STATE_CHECKED);
  g_wifiEnabled = on;
  storeSaveFlags();
  netSetEnabled(on);
  uiSetWifi(g_wifiConnected);                       // refleja el cambio en el icono de Medir
  if (g_lblNetStatus) {
    lv_obj_set_style_text_color(g_lblNetStatus, DV_GREY, 0);
    lv_label_set_text(g_lblNetStatus, on ? netStatusStr().c_str() : "WiFi apagado");
  }
}

// Switch ahorro de bateria: activa/desactiva el reposo por inactividad.
static void dv_pwrSw_cb(lv_event_t* e) {
  g_powerSaveEnabled = lv_obj_has_state(lv_event_get_target(e), LV_STATE_CHECKED);
  storeSaveFlags();
  displaySetSaver(g_powerSaveEnabled);               // brillo 50% al activar / 100% al desactivar
  lv_disp_trig_activity(NULL);                       // reinicia el contador de inactividad
}

static void dv_buildConfig(lv_obj_t* page) {
  dv_pageBase(page);

  // ── Fila de cabecera: version (izq) | titulo (centro) | boton OTA (der) ──
  g_lblFwVer = lv_label_create(page);
  lv_label_set_text(g_lblFwVer, "v" DV_FW_VERSION);
  lv_obj_set_style_text_color(g_lblFwVer, DV_GREY, 0);
  lv_obj_align(g_lblFwVer, LV_ALIGN_TOP_LEFT, 8, 5);

  lv_obj_t* t = lv_label_create(page);
  lv_label_set_text(t, "Config");
  lv_obj_set_style_text_color(t, DV_CYAN, 0);
  lv_obj_align(t, LV_ALIGN_TOP_MID, 0, 4);

  lv_obj_t* bota = dv_borderBtn(page, 40, 22, LV_ALIGN_TOP_RIGHT, -8, 2, dv_ota_cb);
  lv_obj_t* lota = lv_label_create(bota);
  lv_label_set_text(lota, LV_SYMBOL_DOWNLOAD);
  lv_obj_set_style_text_color(lota, DV_CYAN, 0);
  lv_obj_center(lota);

  // ═══════════════ MITAD SUPERIOR: campos que usan el teclado ═══════════════
  // (el teclado cubre la mitad inferior -> los campos deben estar aqui arriba)

  // ── IP del bot ──
  g_taIp = dv_makeTa(page, "IP del bot  (ej: 192.168.1.10)", 28,
                     g_botIp.c_str(), 216, false);

  // ── SSID ──
  g_taSsid = dv_makeTa(page, "SSID", 66, g_ssid.c_str(), 216, false);

  // ── Contrasena + ojo ──
  g_taPass = dv_makeTa(page, "Clave WiFi", 104, g_pass.c_str(), 172, true);
  lv_obj_t* be = dv_borderBtn(page, 36, 34, LV_ALIGN_TOP_LEFT, 192, 104, dv_eye_cb);
  g_lblEye = lv_label_create(be);
  lv_label_set_text(g_lblEye, LV_SYMBOL_EYE_OPEN);
  lv_obj_set_style_text_color(g_lblEye, DV_CYAN, 0);
  lv_obj_center(g_lblEye);

  // ═══════════════ MITAD INFERIOR: botones / switches (sin teclado) ═══════════════

  // ── Fila: 3 switches con icono a la izquierda ──
  // Layout x (240px, margen 12 a c/lado, gap 18 entre grupos):
  //   WiFi  icon=12  sw=30   Charge icon=90  sw=108   BT icon=166  sw=184
  lv_obj_t* lw = lv_label_create(page);
  lv_label_set_text(lw, LV_SYMBOL_WIFI);
  lv_obj_set_style_text_color(lw, DV_CYAN2, 0);
  lv_obj_align(lw, LV_ALIGN_TOP_LEFT, 12, 148);

  g_swWifi = lv_switch_create(page);
  lv_obj_set_size(g_swWifi, 40, 22);
  lv_obj_align(g_swWifi, LV_ALIGN_TOP_LEFT, 30, 145);
  lv_obj_set_style_bg_color(g_swWifi, DV_CYAN, LV_PART_INDICATOR | LV_STATE_CHECKED);
  if (g_wifiConnected) lv_obj_add_state(g_swWifi, LV_STATE_CHECKED);
  lv_obj_add_event_cb(g_swWifi, dv_wifiSw_cb, LV_EVENT_VALUE_CHANGED, NULL);

  lv_obj_t* lah = lv_label_create(page);
  lv_label_set_text(lah, LV_SYMBOL_CHARGE);   // rayo = ahorro de energia
  lv_obj_set_style_text_color(lah, DV_CYAN2, 0);
  lv_obj_align(lah, LV_ALIGN_TOP_LEFT, 88, 148);

  lv_obj_t* spa = lv_switch_create(page);
  lv_obj_set_size(spa, 40, 22);
  lv_obj_align(spa, LV_ALIGN_TOP_LEFT, 106, 145);
  lv_obj_set_style_bg_color(spa, DV_CYAN, LV_PART_INDICATOR | LV_STATE_CHECKED);
  if (g_powerSaveEnabled) lv_obj_add_state(spa, LV_STATE_CHECKED);
  lv_obj_add_event_cb(spa, dv_pwrSw_cb, LV_EVENT_VALUE_CHANGED, NULL);

  lv_obj_t* lbt = lv_label_create(page);
  lv_label_set_text(lbt, LV_SYMBOL_BLUETOOTH);  // bluetooth = emparejar
  lv_obj_set_style_text_color(lbt, DV_CYAN2, 0);
  lv_obj_align(lbt, LV_ALIGN_TOP_LEFT, 164, 148);

  g_swPair = lv_switch_create(page);
  lv_obj_set_size(g_swPair, 40, 22);
  lv_obj_align(g_swPair, LV_ALIGN_TOP_LEFT, 182, 145);
  lv_obj_set_style_bg_color(g_swPair, DV_CYAN, LV_PART_INDICATOR | LV_STATE_CHECKED);
  if (g_deviceId.startsWith("druida_")) lv_obj_add_state(g_swPair, LV_STATE_CHECKED);
  lv_obj_add_event_cb(g_swPair, dv_swPair_cb, LV_EVENT_VALUE_CHANGED, NULL);

  // ── Buscar redes + dropdown ──
  lv_obj_t* bs = dv_borderBtn(page, 216, 24, LV_ALIGN_TOP_LEFT, 12, 176, dv_scan_cb);
  lv_obj_t* lbs = lv_label_create(bs);
  lv_label_set_text(lbs, LV_SYMBOL_REFRESH "  Buscar redes WiFi");
  lv_obj_set_style_text_color(lbs, DV_CYAN, 0);
  lv_obj_center(lbs);

  g_ddNetworks = lv_dropdown_create(page);
  lv_dropdown_set_options(g_ddNetworks, "(Buscar redes)");
  lv_obj_set_width(g_ddNetworks, 216);
  lv_obj_align(g_ddNetworks, LV_ALIGN_TOP_LEFT, 12, 204);
  lv_obj_add_event_cb(g_ddNetworks, dv_dd_cb, LV_EVENT_VALUE_CHANGED, NULL);

  // ── Conectar WiFi ──
  lv_obj_t* btn = dv_borderBtn(page, 216, 24, LV_ALIGN_TOP_MID, 0, 240, dv_connect_cb);
  lv_obj_t* lb = lv_label_create(btn);
  lv_label_set_text(lb, LV_SYMBOL_WIFI "  Conectar WiFi");
  lv_obj_set_style_text_color(lb, DV_CYAN, 0);
  lv_obj_center(lb);

  // status WiFi / resultado OTA
  g_lblNetStatus = lv_label_create(page);
  lv_label_set_text(g_lblNetStatus, "-");
  lv_obj_set_style_text_color(g_lblNetStatus, DV_GREY, 0);
  lv_obj_set_style_text_align(g_lblNetStatus, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_align(g_lblNetStatus, LV_ALIGN_TOP_MID, 0, 268);
  g_lblOta = g_lblNetStatus;

  // barra de progreso OTA (reemplaza el texto durante la descarga)
  g_otaBar = lv_bar_create(page);
  lv_obj_set_size(g_otaBar, 216, 12);
  lv_obj_align(g_otaBar, LV_ALIGN_TOP_MID, 0, 268);
  lv_bar_set_range(g_otaBar, 0, 100);
  lv_bar_set_value(g_otaBar, 0, LV_ANIM_OFF);
  lv_obj_set_style_bg_color(g_otaBar, DV_PANEL, LV_PART_MAIN);
  lv_obj_set_style_border_color(g_otaBar, DV_GREY, LV_PART_MAIN);
  lv_obj_set_style_border_width(g_otaBar, 1, LV_PART_MAIN);
  lv_obj_set_style_radius(g_otaBar, 6, LV_PART_MAIN);
  lv_obj_set_style_bg_color(g_otaBar, DV_CYAN, LV_PART_INDICATOR);
  lv_obj_set_style_radius(g_otaBar, 6, LV_PART_INDICATOR);
  lv_obj_add_flag(g_otaBar, LV_OBJ_FLAG_HIDDEN);  // oculta hasta que empiece la descarga
}

// ───────────────────────── OTA (firmware) ─────────────────────────
static void dv_ota_cb(lv_event_t* e) {
  if (!g_wifiConnected) {                       // sin WiFi no hay como descargar
    if (g_lblOta) {
      lv_obj_set_style_text_color(g_lblOta, lv_palette_main(LV_PALETTE_RED), 0);
      lv_label_set_text(g_lblOta, "Sin WiFi: conecta en Config");
    }
    return;
  }
  if (g_otaBar) {
    lv_bar_set_value(g_otaBar, 0, LV_ANIM_OFF);
    lv_obj_clear_flag(g_otaBar, LV_OBJ_FLAG_HIDDEN);
  }
  if (g_lblOta) {
    lv_obj_set_style_text_color(g_lblOta, DV_CYAN2, 0);
    lv_label_set_text(g_lblOta, "Buscando firmware...");
  }
  g_otaPending = true;                          // lo atiende el loop (bloqueante)
}

void uiSetWifi(bool connected) {
  lv_obj_set_style_text_color(g_lblWifi,
    connected ? lv_palette_main(LV_PALETTE_GREEN) : DV_GREY, 0);
  // El switch de Config refleja "conectado a internet": se prende solo al
  // confirmar conexion, y se apaga solo cuando la radio queda apagada (sea por
  // el usuario o porque netLoop la apago tras perder/fallar la conexion). Si
  // esta "Conectando..." (radio prendida, aun sin confirmar) no se toca, para no
  // revertir el toggle que el usuario recien hizo.
  if (g_swWifi) {
    if (connected)            lv_obj_add_state  (g_swWifi, LV_STATE_CHECKED);
    else if (!g_wifiEnabled)  lv_obj_clear_state(g_swWifi, LV_STATE_CHECKED);
  }
}

// Actualiza el icono de bateria desde g_batState / g_batMv / g_batPct (battery.h)
void uiSetBattery() {
  if (!g_lblBatt) return;
  lv_color_t  col;
  const char* glyph;
  bool        slash = false;

  if (g_batState == DV_PWR_NONE) {                 // sin bateria => vacio + tachado (rojo)
    glyph = LV_SYMBOL_BATTERY_EMPTY;
    col   = lv_palette_main(LV_PALETTE_RED);
    slash = true;
  } else if (g_batState == DV_PWR_CHARGING) {      // cargando => rayo (verde)
    glyph = LV_SYMBOL_CHARGE;
    col   = lv_palette_main(LV_PALETTE_GREEN);
  } else {                                         // en bateria => nivel por %
    int p = g_batPct;
    if      (p >= 80) glyph = LV_SYMBOL_BATTERY_FULL;
    else if (p >= 55) glyph = LV_SYMBOL_BATTERY_3;
    else if (p >= 35) glyph = LV_SYMBOL_BATTERY_2;
    else if (p >= 12) glyph = LV_SYMBOL_BATTERY_1;
    else              glyph = LV_SYMBOL_BATTERY_EMPTY;
    col = (p >= 50) ? lv_palette_main(LV_PALETTE_GREEN)
        : (p >= 20) ? lv_palette_main(LV_PALETTE_ORANGE)
                    : lv_palette_main(LV_PALETTE_RED);
  }

  lv_label_set_text(g_lblBatt, glyph);
  lv_obj_set_style_text_color(g_lblBatt, col, 0);

  if (g_lblBatVolt) {
    char t[12];
    if (g_batState == DV_PWR_NONE) snprintf(t, sizeof(t), "--");
    else snprintf(t, sizeof(t), "%d.%02dV", g_batMv / 1000, (g_batMv % 1000) / 10);
    lv_label_set_text(g_lblBatVolt, t);
    lv_obj_set_style_text_color(g_lblBatVolt, col, 0);
  }

  if (g_battSlash) {
    if (slash) lv_obj_clear_flag(g_battSlash, LV_OBJ_FLAG_HIDDEN);
    else       lv_obj_add_flag(g_battSlash, LV_OBJ_FLAG_HIDDEN);
  }
}
void uiSetNetStatus(const char* txt) {
  if (!g_lblNetStatus) return;
  lv_obj_set_style_text_color(g_lblNetStatus, DV_GREY, 0);  // limpia color que dejo el OTA
  lv_label_set_text(g_lblNetStatus, txt);
}
void uiSetNetworks(const char* opts) {
  lv_dropdown_set_options(g_ddNetworks, (opts && opts[0]) ? opts : "(sin redes)");
}
void uiRefreshCal() { dv_updateCalLabels(); }   // re-sincroniza labels (cambios via web)

void uiSetCloudMsg(const char* txt) {
  if (g_lblCloudMsg) lv_label_set_text(g_lblCloudMsg, txt);
}
// code = codigo HTTP (200 = OK) o centinela de cloud.h (-1000 sin devid, -1001 sin wifi, -1002 begin)
void uiCloudResult(int code, int val, const char* kind, int samples) {
  bool ok = (code == 200);
  if (g_lblCloud)   // FREE: el icono de nube no existe -> ruta inerte, evita null-deref
    lv_obj_set_style_text_color(g_lblCloud,
      ok ? lv_palette_main(LV_PALETTE_GREEN) : lv_palette_main(LV_PALETTE_RED), 0);
  char b[56];
  if (ok) {
    if (strcmp(kind, "avg") == 0) snprintf(b, sizeof(b), "Subido %d (prom %d)", val, samples);
    else                          snprintf(b, sizeof(b), "Subido %d (unica)", val);
  } else if (code == -1000) {     // DV_CLOUD_NO_DEVID
    snprintf(b, sizeof(b), "Falta device_id");
  } else if (code == -1001) {     // DV_CLOUD_NO_WIFI
    snprintf(b, sizeof(b), "Sin WiFi");
  } else if (code == -1002) {     // DV_CLOUD_NO_BEGIN
    snprintf(b, sizeof(b), "Error de conexion");
  } else if (code == 404) {
    snprintf(b, sizeof(b), "Error 404 (device_id?)");
  } else if (code == 500) {
    snprintf(b, sizeof(b), "Error 500 (SQL?)");
  } else if (code < 0) {
    snprintf(b, sizeof(b), "Error de red (%d)", code);
  } else {
    snprintf(b, sizeof(b), "Error nube (HTTP %d)", code);
  }
  if (g_lblCloudMsg) lv_label_set_text(g_lblCloudMsg, b);
}
// limpia el buffer manual sin tocar el mensaje (lo llama el loop tras subir el promedio)
void uiClearBuffer() { g_capN = 0; g_capSum = 0; }

// Resultado del emparejamiento (lo llama el loop tras consultar /api/info del bot).
// code = HTTP (200 OK) o centinela: -1000 falta IP, -1001 sin wifi, -2000 bot sin device_id.
void uiPairResult(int code, const char* devid) {
  bool ok = (code == 200 && devid && strncmp(devid, "druida_", 7) == 0);
  if (g_swPair) {                            // switch: ON=emparejado, OFF=fallo/sin par
    if (ok) lv_obj_add_state   (g_swPair, LV_STATE_CHECKED);
    else    lv_obj_clear_state (g_swPair, LV_STATE_CHECKED);
  }
  if (!g_lblNetStatus) return;               // detalle (una vez) en la linea de estado
  char b[64];
  lv_color_t col = lv_palette_main(LV_PALETTE_RED);
  if (ok)                  { snprintf(b, sizeof(b), "Emparejado: %s", devid);
                             col = lv_palette_main(LV_PALETTE_GREEN); }
  else if (code == -1000)  { snprintf(b, sizeof(b), "Falta la IP del bot"); }
  else if (code == -1001)  { snprintf(b, sizeof(b), "Sin WiFi"); }
  else if (code == -2000)  { snprintf(b, sizeof(b), "Bot sin device_id (OTA)"); }
  else if (code == -2001)  { snprintf(b, sizeof(b), "Respuesta rara del bot"); }
  else if (code <= 0)      { snprintf(b, sizeof(b), "Bot no responde"); }
  else                     { snprintf(b, sizeof(b), "Error (HTTP %d)", code); }
  lv_obj_set_style_text_color(g_lblNetStatus, col, 0);
  lv_label_set_text(g_lblNetStatus, b);
}

// ── OTA: hooks llamados desde ota.h / el loop ──
// Avance de la descarga (desde el callback de httpUpdate, dentro del bloqueo).
void uiOtaProgress(int pct) {
  if (g_otaBar) {
    lv_obj_clear_flag(g_otaBar, LV_OBJ_FLAG_HIDDEN);
    lv_bar_set_value(g_otaBar, pct, LV_ANIM_OFF);
  }
  if (g_lblOta) lv_obj_add_flag(g_lblOta, LV_OBJ_FLAG_HIDDEN);  // oculta texto durante descarga
  lv_refr_now(NULL);                 // redibuja ya (estamos dentro de la descarga bloqueante)
}

void uiOtaSetMsg(const char* msg) {
  if (!g_lblOta) return;
  lv_obj_set_style_text_color(g_lblOta, DV_CYAN2, 0);
  lv_label_set_text(g_lblOta, msg);
}

// r: DV_OTA_NOUPD / DV_OTA_FAIL / DV_OTA_NOWIFI  (DV_OTA_OK reinicia, no llega aca)
void uiOtaResult(int r) {
  if (g_otaBar) lv_obj_add_flag(g_otaBar, LV_OBJ_FLAG_HIDDEN);
  if (g_lblOta) lv_obj_clear_flag(g_lblOta, LV_OBJ_FLAG_HIDDEN);  // restaura texto
  lv_color_t col = lv_palette_main(LV_PALETTE_RED);
  char b[80];
  if (r == DV_OTA_NOUPD) {
    col = lv_palette_main(LV_PALETTE_GREEN);
    snprintf(b, sizeof(b), "Ya estas al dia (sin cambios)");
  } else if (r == DV_OTA_NOWIFI) {
    snprintf(b, sizeof(b), "Sin WiFi: conecta en Config");
  } else {                                          // DV_OTA_FAIL
    if (g_otaErr == 404)
      snprintf(b, sizeof(b), "Firmware no encontrado en el repo\n(falta subir el .bin)");
    else
      snprintf(b, sizeof(b), "Error %d\n%.32s", g_otaErr, g_otaErrStr.c_str());
  }
  lv_obj_set_style_text_color(g_lblOta, col, 0);
  lv_label_set_text(g_lblOta, b);
}

// ───────────────────────── Dim (control dimmer) ─────────────────────────

// Refresca el label del setpoint con el valor actual
static void dv_dimRefreshSetpoint() {
  if (g_lblDimSetpoint)
    lv_label_set_text_fmt(g_lblDimSetpoint, "%d", g_dimSetpoint);
}

// ── Selector de canal (popup 1-8) ────────────────────────────────────────────

// Selección de canal: actualiza g_dimChannel y cierra el popup.
static void dv_dimChSel_cb(lv_event_t* e) {
  int ch = (intptr_t)lv_event_get_user_data(e);
  g_dimChannel = ch;
  if (g_dimChLabel) lv_label_set_text_fmt(g_dimChLabel, "CH %d", g_dimChannel);
  // Cerrar overlay: btn → panel → overlay
  lv_obj_t* btn     = lv_event_get_target(e);
  lv_obj_t* panel   = lv_obj_get_parent(btn);
  lv_obj_t* overlay = lv_obj_get_parent(panel);
  lv_obj_del(overlay);
}

// Tap en el overlay fuera del panel → cierra sin cambiar selección.
static void dv_dimOverlayTap_cb(lv_event_t* e) {
  lv_obj_t* overlay = (lv_obj_t*)lv_event_get_user_data(e);
  if (lv_event_get_target(e) == overlay)   // no propagar desde hijos
    lv_obj_del(overlay);
}

// Abre el popup de selección de canal (1-8).
static void dv_dimChOpen_cb(lv_event_t* e) {
  // Overlay semitransparente sobre toda la pantalla
  lv_obj_t* ov = lv_obj_create(lv_scr_act());
  lv_obj_set_size(ov, LV_HOR_RES, LV_VER_RES);
  lv_obj_align(ov, LV_ALIGN_TOP_LEFT, 0, 0);
  lv_obj_set_style_bg_color(ov, lv_color_black(), 0);
  lv_obj_set_style_bg_opa(ov, LV_OPA_70, 0);
  lv_obj_set_style_border_width(ov, 0, 0);
  lv_obj_set_style_pad_all(ov, 0, 0);
  lv_obj_clear_flag(ov, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_event_cb(ov, dv_dimOverlayTap_cb, LV_EVENT_CLICKED, ov);

  // Panel central con los 8 botones
  lv_obj_t* panel = lv_obj_create(ov);
  lv_obj_set_size(panel, 188, 142);
  lv_obj_center(panel);
  lv_obj_set_style_bg_color(panel, DV_PANEL, 0);
  lv_obj_set_style_border_color(panel, DV_CYAN, 0);
  lv_obj_set_style_border_width(panel, 2, 0);
  lv_obj_set_style_pad_all(panel, 6, 0);
  lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t* title = lv_label_create(panel);
  lv_label_set_text(title, "Canal de salida");
  lv_obj_set_style_text_color(title, DV_WHITE, 0);
  lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 2);

  // 8 botones en 2 filas × 4 columnas
  for (int i = 0; i < 8; i++) {
    int col = i % 4;
    int row = i / 4;
    lv_obj_t* b = lv_btn_create(panel);
    lv_obj_set_size(b, 40, 42);
    lv_obj_align(b, LV_ALIGN_TOP_LEFT, col * 44, 26 + row * 48);
    bool sel = ((i + 1) == g_dimChannel);
    lv_obj_set_style_bg_color(b, sel ? DV_CYAN : DV_PANEL, 0);
    lv_obj_set_style_border_color(b, DV_CYAN, 0);
    lv_obj_set_style_border_width(b, 2, 0);
    lv_obj_add_event_cb(b, dv_dimChSel_cb, LV_EVENT_CLICKED, (void*)(intptr_t)(i + 1));
    lv_obj_t* l = lv_label_create(b);
    lv_label_set_text_fmt(l, "%d", i + 1);
    lv_obj_set_style_text_color(l, sel ? DV_PANEL : DV_CYAN, 0);
    lv_obj_center(l);
  }
}

// ── Botones +/− del setpoint
//   tag: 0=−100  1=−10  2=+10  3=+100
static void dv_dimStep_cb(lv_event_t* e) {
  static uint32_t lastStep = 0;
  uint32_t now = millis();
  if (now - lastStep < 150) return;   // debounce suave (tap rapido ok, muy rapido no)
  lastStep = now;

  intptr_t tag = (intptr_t)lv_event_get_user_data(e);
  switch (tag) {
    case 0: g_dimSetpoint -= 100; break;
    case 1: g_dimSetpoint -= 10;  break;
    case 2: g_dimSetpoint += 10;  break;
    case 3: g_dimSetpoint += 100; break;
  }
  if (g_dimSetpoint < 0)    g_dimSetpoint = 0;
  if (g_dimSetpoint > 2000) g_dimSetpoint = 2000;
  dv_dimRefreshSetpoint();
  storeSaveDim();   // persiste en NVS cada cambio
}

// Boton Enviar: arma el pedido (bloqueante, lo atiende el loop principal)
static void dv_dimEnviar_cb(lv_event_t* e) {
  static uint32_t lastSend = 0;
  // 400ms: debounce del tap corto y ritmo de la presion sostenida (envio continuo).
  // El HTTP bloqueante de dimmerSendSetpoint() ya limita el ritmo real.
  if (millis() - lastSend < 400) return;
  lastSend = millis();

  if (!g_wifiConnected) {
    if (g_lblDimStatus) {
      lv_obj_set_style_text_color(g_lblDimStatus, lv_palette_main(LV_PALETTE_RED), 0);
      lv_label_set_text(g_lblDimStatus, "Sin WiFi");
    }
    return;
  }
  if (g_botIp.length() == 0) {
    if (g_lblDimStatus) {
      lv_obj_set_style_text_color(g_lblDimStatus, lv_palette_main(LV_PALETTE_RED), 0);
      lv_label_set_text(g_lblDimStatus, "Bot no emparejado (Config)");
    }
    return;
  }
  if (g_lblDimStatus) {
    lv_obj_set_style_text_color(g_lblDimStatus, DV_CYAN2, 0);
    lv_label_set_text(g_lblDimStatus, "Enviando...");
  }
  g_dimPending = true;   // el loop llama dimmerSendSetpoint() y luego uiDimResult()
}

static void dv_buildDim(lv_obj_t* page) {
  dv_pageBase(page);

  // ── Título ──
  lv_obj_t* t = lv_label_create(page);
  lv_label_set_text(t, "Dim");
  lv_obj_set_style_text_color(t, DV_CYAN, 0);
  lv_obj_align(t, LV_ALIGN_TOP_MID, 0, 4);

  // ── PPFD en vivo ──
  lv_obj_t* lLive = lv_label_create(page);
  lv_label_set_text(lLive, "PPFD en vivo");
  lv_obj_set_style_text_color(lLive, DV_GREY, 0);
  lv_obj_align(lLive, LV_ALIGN_TOP_MID, 0, 22);

  g_lblDimLive = lv_label_create(page);
  lv_obj_set_style_text_font(g_lblDimLive, DV_MED_FONT, 0);
  lv_obj_set_style_text_color(g_lblDimLive, DV_WHITE, 0);
  lv_label_set_text(g_lblDimLive, "--");
  lv_obj_align(g_lblDimLive, LV_ALIGN_TOP_MID, 0, 36);

  lv_obj_t* uLive = lv_label_create(page);
  lv_label_set_text(uLive, "umol/m" LV_SYMBOL_DUMMY "s");
  lv_obj_set_style_text_color(uLive, DV_CYAN2, 0);
  lv_obj_align(uLive, LV_ALIGN_TOP_MID, 0, 78);

  // ── Setpoint ──
  lv_obj_t* lSp = lv_label_create(page);
  lv_label_set_text(lSp, "Setpoint objetivo");
  lv_obj_set_style_text_color(lSp, DV_CYAN2, 0);
  lv_obj_align(lSp, LV_ALIGN_TOP_MID, 0, 100);

  g_lblDimSetpoint = lv_label_create(page);
  lv_obj_set_style_text_font(g_lblDimSetpoint, DV_MED_FONT, 0);
  lv_obj_set_style_text_color(g_lblDimSetpoint, DV_WHITE, 0);
  dv_dimRefreshSetpoint();
  lv_obj_align(g_lblDimSetpoint, LV_ALIGN_TOP_MID, 0, 114);

  lv_obj_t* uSp = lv_label_create(page);
  lv_label_set_text(uSp, "umol/m" LV_SYMBOL_DUMMY "s");
  lv_obj_set_style_text_color(uSp, DV_GREY, 0);
  lv_obj_align(uSp, LV_ALIGN_TOP_MID, 0, 156);

  // ── 4 botones de paso: −100  −10  +10  +100 ──
  // Ancho total: 4×52 + 3×6 gap = 226 px  →  offset inicial x=7
  static const char*   stepLbl[] = { "-100", "-10", "+10", "+100" };
  static const int     stepX[]   = { 7, 65, 123, 181 };
  static const intptr_t stepTag[] = { 0, 1, 2, 3 };
  for (int i = 0; i < 4; i++) {
    lv_obj_t* b = lv_btn_create(page);
    lv_obj_set_size(b, 52, 36);
    lv_obj_align(b, LV_ALIGN_TOP_LEFT, stepX[i], 172);
    lv_obj_set_style_bg_color(b, DV_PANEL, 0);
    lv_obj_set_style_border_color(b, DV_CYAN, 0);
    lv_obj_set_style_border_width(b, 2, 0);
    lv_obj_add_event_cb(b, dv_dimStep_cb, LV_EVENT_CLICKED, (void*)stepTag[i]);
    lv_obj_t* l = lv_label_create(b);
    lv_label_set_text(l, stepLbl[i]);
    lv_obj_set_style_text_color(l, DV_CYAN, 0);
    lv_obj_center(l);
  }

  // ── Fila y=216: [DIMMERIZAR 155px] [CH N 52px] ──
  // Tap corto en DIMMERIZAR = un envío. MANTENER PRESIONADO = envíos continuos hasta objetivo.
  // Botón DIMMERIZAR (izquierda, grande y fácil de apretar)
  lv_obj_t* bEnv = dv_borderBtn(page, 155, 56, LV_ALIGN_TOP_LEFT, 14, 216, dv_dimEnviar_cb);
  lv_obj_add_event_cb(bEnv, dv_dimEnviar_cb, LV_EVENT_LONG_PRESSED_REPEAT, NULL);
  lv_obj_t* lEnv = lv_label_create(bEnv);
  lv_label_set_text(lEnv, "DIMMERIZAR");
  lv_obj_set_style_text_font(lEnv, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(lEnv, DV_CYAN, 0);
  lv_obj_center(lEnv);

  // Botón selector de canal (derecha, misma fila)
  lv_obj_t* bCh = dv_borderBtn(page, 52, 56, LV_ALIGN_TOP_LEFT, 175, 216, dv_dimChOpen_cb);
  g_dimChLabel = lv_label_create(bCh);
  lv_label_set_text_fmt(g_dimChLabel, "CH %d", g_dimChannel);
  lv_obj_set_style_text_font(g_dimChLabel, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(g_dimChLabel, DV_CYAN, 0);
  lv_obj_center(g_dimChLabel);

  // ── Status (solo para errores; en éxito queda en blanco) ──
  g_lblDimStatus = lv_label_create(page);
  lv_label_set_text(g_lblDimStatus, "");
  lv_obj_set_style_text_color(g_lblDimStatus, DV_GREY, 0);
  lv_obj_set_style_text_align(g_lblDimStatus, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_width(g_lblDimStatus, 220);
  lv_obj_align(g_lblDimStatus, LV_ALIGN_TOP_MID, 0, 280);
}

// Resultado del envío al dimmer (llamado desde el loop principal)
void uiDimResult(int code) {
  if (!g_lblDimStatus) return;
  char b[48];
  lv_color_t col;
  if (code == 200) {
    // Éxito: no mostramos mensaje (el botón ocupa ese espacio). Limpiamos la línea.
    lv_label_set_text(g_lblDimStatus, "");
    return;
  } else if (code == DV_DIM_NO_IP) {   // -2000
    snprintf(b, sizeof(b), "Bot no emparejado (Config)");
    col = lv_palette_main(LV_PALETTE_RED);
  } else if (code == DV_DIM_NO_WIFI) { // -2001
    snprintf(b, sizeof(b), "Sin WiFi");
    col = lv_palette_main(LV_PALETTE_RED);
  } else if (code == 404) {
    snprintf(b, sizeof(b), "Bot: endpoint no disponible aun");
    col = lv_palette_main(LV_PALETTE_ORANGE);
  } else {
    snprintf(b, sizeof(b), "Error (%d)", code);
    col = lv_palette_main(LV_PALETTE_RED);
  }
  lv_obj_set_style_text_color(g_lblDimStatus, col, 0);
  lv_label_set_text(g_lblDimStatus, b);
}

// Actualiza el PPFD en vivo de la pestaña Dim (llamado desde uiUpdate)
void uiDimUpdateLive(float ppfd) {
  // Guarda la última lectura válida para enviarla junto al setpoint (ver dimmer.h)
  g_dimLivePpfd = (isnan(ppfd) || ppfd < 0) ? -1 : (int)lroundf(ppfd);
  if (!g_lblDimLive) return;
  if (g_dimLivePpfd < 0)
    lv_label_set_text(g_lblDimLive, "--");
  else
    lv_label_set_text_fmt(g_lblDimLive, "%d", g_dimLivePpfd);
}

// ─────────────────── splash de bienvenida al salir del reposo ───────────────────
static void dv_splashOpa_cb(void* obj, int32_t v) {
  lv_obj_set_style_opa((lv_obj_t*)obj, (lv_opa_t)v, 0);
}
static void dv_splashDel_cb(lv_anim_t* a) {
  lv_obj_del((lv_obj_t*)a->var);
}

// Lanza un fundido de entrada (transparente -> opaco) en `obj`, arrancando luego
// de `delay_ms`. Usado para ir revelando cada linea del splash en secuencia.
static void dv_splashFadeIn(lv_obj_t* obj, uint32_t delay_ms, uint32_t dur_ms) {
  lv_obj_set_style_opa(obj, LV_OPA_TRANSP, 0);
  lv_anim_t a;
  lv_anim_init(&a);
  lv_anim_set_var(&a, obj);
  lv_anim_set_exec_cb(&a, dv_splashOpa_cb);
  lv_anim_set_values(&a, LV_OPA_TRANSP, LV_OPA_COVER);
  lv_anim_set_time(&a, dur_ms);
  lv_anim_set_delay(&a, delay_ms);
  lv_anim_start(&a);
}

// Logo "DATA / DRUIDA" + URL revelandose en secuencia (DATA -> a 1.5s DRUIDA ->
// luego la URL), mostrado al despertar del modo ahorro (powerWake en power.h).
// Dura ~4.6 s en total y se autodestruye al terminar el fundido final.
void uiShowWakeSplash() {
  lv_obj_t* ov = lv_obj_create(lv_scr_act());
  lv_obj_remove_style_all(ov);
  lv_obj_set_size(ov, LV_PCT(100), LV_PCT(100));
  lv_obj_set_style_bg_color(ov, DV_BG, 0);
  lv_obj_set_style_bg_opa(ov, LV_OPA_COVER, 0);
  lv_obj_clear_flag(ov, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_clear_flag(ov, LV_OBJ_FLAG_CLICKABLE);  // no debe robar el toque que reactiva la UI

  lv_obj_t* lData = lv_label_create(ov);
  lv_label_set_text(lData, "DATA");
  lv_obj_set_style_text_align(lData, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_color(lData, DV_CYAN, 0);
  lv_obj_set_style_text_font(lData, DV_MED_FONT, 0);
  lv_obj_align(lData, LV_ALIGN_CENTER, 0, -34);

  lv_obj_t* lDruida = lv_label_create(ov);
  lv_label_set_text(lDruida, "DRUIDA");
  lv_obj_set_style_text_align(lDruida, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_color(lDruida, DV_CYAN, 0);
  lv_obj_set_style_text_font(lDruida, DV_MED_FONT, 0);
  lv_obj_align(lDruida, LV_ALIGN_CENTER, 0, -2);

  lv_obj_t* lUrl = lv_label_create(ov);
  lv_label_set_text(lUrl, "www.datadruida.com.ar");
  lv_obj_set_style_text_align(lUrl, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_color(lUrl, DV_CYAN2, 0);
  lv_obj_align(lUrl, LV_ALIGN_CENTER, 0, 36);

  // secuencia de aparicion: DATA (t=0) -> DRUIDA (t=1.5s) -> URL (t=3.2s)
  dv_splashFadeIn(lData,   0,    400);
  dv_splashFadeIn(lDruida, 1500, 400);
  dv_splashFadeIn(lUrl,    3200, 400);

  // a los 4.2s todo el splash (fondo + textos) se desvanece junto y se borra
  lv_anim_t out;
  lv_anim_init(&out);
  lv_anim_set_var(&out, ov);
  lv_anim_set_exec_cb(&out, dv_splashOpa_cb);
  lv_anim_set_values(&out, LV_OPA_COVER, LV_OPA_TRANSP);
  lv_anim_set_time(&out, 400);
  lv_anim_set_delay(&out, 4200);
  lv_anim_set_ready_cb(&out, dv_splashDel_cb);
  lv_anim_start(&out);
}

// ─────────────── splash de "bateria baja" -> reposo forzado ───────────────
// El equipo no tiene interruptor fisico para "apagarse": dormirlo (powerEnterSleep,
// que apaga pantalla + WiFi) es la unica forma. Asi que, como hace un celular,
// antes de dormir por bateria critica mostramos un aviso claro tipo "Recargar
// bateria" y recien terminada la animacion pedimos el reposo (g_battCriticalSleep,
// lo atiende el loop igual que g_sleepRequested).
static void dv_battSplashDone_cb(lv_anim_t* a) {
  lv_obj_del((lv_obj_t*)a->var);
  g_battCriticalSleep = true;             // el loop entra en reposo (powerEnterSleep)
}

void uiShowLowBatterySplash() {
  lv_obj_t* ov = lv_obj_create(lv_scr_act());
  lv_obj_remove_style_all(ov);
  lv_obj_set_size(ov, LV_PCT(100), LV_PCT(100));
  lv_obj_set_style_bg_color(ov, DV_BG, 0);
  lv_obj_set_style_bg_opa(ov, LV_OPA_COVER, 0);
  lv_obj_clear_flag(ov, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_clear_flag(ov, LV_OBJ_FLAG_CLICKABLE);  // que no robe toques (igual que el splash de despertar)

  // glifo grande de bateria vacia, en rojo (mismo "lenguaje visual" que un celular)
  lv_obj_t* lIcon = lv_label_create(ov);
  lv_label_set_text(lIcon, LV_SYMBOL_BATTERY_EMPTY);
  lv_obj_set_style_text_color(lIcon, lv_palette_main(LV_PALETTE_RED), 0);
  lv_obj_set_style_text_font(lIcon, DV_PPFD_FONT, 0);
  lv_obj_align(lIcon, LV_ALIGN_CENTER, 0, -58);

  lv_obj_t* lTitle = lv_label_create(ov);
  lv_label_set_text(lTitle, "Bateria baja");
  lv_obj_set_style_text_align(lTitle, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_color(lTitle, DV_WHITE, 0);
  lv_obj_set_style_text_font(lTitle, DV_MED_FONT, 0);
  lv_obj_align(lTitle, LV_ALIGN_CENTER, 0, 14);

  lv_obj_t* lSub = lv_label_create(ov);
  lv_label_set_text(lSub, "Conecta el cargador\npara seguir usando el equipo");
  lv_obj_set_style_text_align(lSub, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_color(lSub, DV_GREY, 0);
  lv_obj_align(lSub, LV_ALIGN_CENTER, 0, 56);

  // se queda ~4.5s en pantalla (tiempo de sobra para leerlo) y se desvanece ->
  // recien ahi pide el reposo, asi el usuario alcanza a ver el aviso completo
  lv_anim_t out;
  lv_anim_init(&out);
  lv_anim_set_var(&out, ov);
  lv_anim_set_exec_cb(&out, dv_splashOpa_cb);
  lv_anim_set_values(&out, LV_OPA_COVER, LV_OPA_TRANSP);
  lv_anim_set_time(&out, 500);
  lv_anim_set_delay(&out, 4500);
  lv_anim_set_ready_cb(&out, dv_battSplashDone_cb);
  lv_anim_start(&out);
}

// ───────────────────────── construir toda la UI ─────────────────────────
void uiInit() {
  lv_obj_t* scr = lv_scr_act();
  lv_obj_set_style_bg_color(scr, DV_BG, 0);

  lv_obj_t* tv = lv_tabview_create(scr, LV_DIR_BOTTOM, 38);
  lv_obj_set_style_bg_color(tv, DV_BG, 0);
  lv_obj_add_event_cb(tv, dv_tabChange_cb, LV_EVENT_VALUE_CHANGED, NULL);  // cambia de pestaña -> cierra el menu Paneles LED si estaba abierto

  lv_obj_t* btns = lv_tabview_get_tab_btns(tv);
  lv_obj_set_style_bg_color(btns, DV_PANEL, 0);
  lv_obj_set_style_text_color(btns, lv_color_hex(0x9fb3c8), LV_PART_ITEMS);
  lv_obj_set_style_text_color(btns, DV_CYAN, LV_PART_ITEMS | LV_STATE_CHECKED);
  lv_obj_set_style_border_color(btns, DV_CYAN, LV_PART_ITEMS | LV_STATE_CHECKED);

  dv_buildMedir (lv_tabview_add_tab(tv, "Medir"));
  dv_buildDLI   (lv_tabview_add_tab(tv, "DLI"));
  dv_buildCalib (lv_tabview_add_tab(tv, "Calib"));
  // FREE: sin pestañas Config (WiFi/OTA/pairing/ahorro) ni Dim (dimmer del bot).
  // Sus builders siguen definidos pero no se invocan (codigo inerte).

  dv_buildPanelMenu(scr);   // overlay "Paneles LED" (oculto; lo abre el boton en Calib)

  // teclado en pantalla (oculto hasta enfocar un campo de texto)
  g_kb = lv_keyboard_create(scr);
  lv_obj_add_flag(g_kb, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_event_cb(g_kb, dv_kb_cb, LV_EVENT_ALL, NULL);

  dv_recalcDLI();
}

// ───────────────────────── update por lectura ─────────────────────────
void uiUpdate(const Reading& rd) {
  dv_show(g_lblSensor, !g_tcsOK);          // "S" roja arriba si no hay sensor

  if (!g_tcsOK) {
    lv_label_set_text(g_lblPpfd, "--");
    lv_label_set_text(g_lblPct, "Sensor no detectado");
    if (!g_showCtrls) dv_drawSpectrum(0.0f);
    return;
  }
  g_uiPpfd = rd.ppfd;
  lv_label_set_text_fmt(g_lblPpfd, "%d", (int)lroundf(rd.ppfd));
  char b[48];
  snprintf(b, sizeof(b), "R %.0f%%   G %.0f%%   B %.0f%%", rd.pr, rd.pg, rd.pb);
  lv_label_set_text(g_lblPct, b);

  // pesos de color normalizados al canal dominante -> reforman la curva por R/G/B
  float m = rd.pr; if (rd.pg > m) m = rd.pg; if (rd.pb > m) m = rd.pb;
  if (m < 1.0f) { g_specWR = g_specWG = g_specWB = 1.0f; }   // sin color claro => silueta full
  else { g_specWR = rd.pr / m; g_specWG = rd.pg / m; g_specWB = rd.pb / m; }

  if (!g_showCtrls) {                       // redibuja el espectro solo si esta visible
    float amp = rd.ppfd / DV_SPEC_FULL_PPFD;
    if (amp < 0.05f) amp = 0.05f;
    if (amp > 1.0f)  amp = 1.0f;
    dv_drawSpectrum(amp);
  }
  dv_recalcDLI();
  uiDimUpdateLive(rd.ppfd);               // sincroniza el PPFD en vivo de la pestaña Dim
}
