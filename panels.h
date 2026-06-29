/*  panels.h — Base de calibracion por marca/modelo de panel LED (EMBEBIDA).
 *
 *  Cada entrada {brand, model, factor, offset}: al elegir un panel en el menu
 *  "Paneles LED" (pestaña Calibracion) se aplican su factor/offset como la
 *  calibracion activa (g_factor/g_offset, persistidas por storeSaveCal — eso lo
 *  hace la UI en ui.h/store.h).
 *
 *  La lista VIVE EN EL FIRMWARE: el array DV_PANELS_LIST esta en panels_data.h,
 *  un archivo GENERADO por la webApp (/admin/paneles-led, boton "Exportar .h").
 *  NO se descarga ni se cachea en ningun lado: viaja dentro del .bin, asi que se
 *  actualiza junto con el firmware en cada OTA. Una sola actualizacion
 *  (firmware + calibraciones).
 *
 *  AGREGAR / EDITAR CALIBRACIONES (no se tocan a mano):
 *    1. Editar los paneles en la webApp  /admin/paneles-led.
 *    2. Boton "Exportar .h" -> descarga panels_data.h y reemplazarlo en el proyecto.
 *    3. Recompilar y publicar el OTA (push_druidavision.bat).
 *    Los equipos reciben las calibraciones nuevas al actualizar el firmware.
 *
 *  [Historico: antes la lista se descargaba de la webapp y se cacheaba en FFAT
 *   (/panels.json), luego en NVS. Se ELIMINO todo eso: el FAT se corrompia ante
 *   cortes de energia y brickeaba el equipo (sin boton). Embeber la lista quita
 *   ese vector por completo, no necesita red y simplifica la operacion.]
 */
#pragma once
#include <Arduino.h>
#include <vector>
#include "store.h"     // g_calPanelB/M + g_calCustom + g_factor/g_offset (calib auto del panel)

// Entrada activa en memoria (con String, la consume la UI).
struct DvPanel { String brand; String model; float factor; float offset; };

// Definicion embebida (POD con const char*: vive en flash, facil de generar).
struct DvPanelDef { const char* brand; const char* model; float factor; float offset; };

std::vector<DvPanel> g_panels;    // lista activa en memoria (cargada al boot de DV_PANELS_LIST)

// La lista en si (array DV_PANELS_LIST + DV_PANELS_GEN) la genera la webApp.
// Va DESPUES de struct DvPanelDef: ese archivo lo usa.
#include "panels_data.h"

// Carga la lista embebida al arrancar. No toca NVS ni FFAT: la lista viaja dentro
// del firmware, asi que NUNCA se corrompe ni puede crashear el arranque, y se
// actualiza junto con el firmware en cada OTA.
void panelsInit() {
  g_panels.clear();
  g_panels.reserve(sizeof(DV_PANELS_LIST) / sizeof(DV_PANELS_LIST[0]));
  for (auto& d : DV_PANELS_LIST) {
    DvPanel p;
    p.brand  = d.brand;
    p.model  = d.model;
    p.factor = d.factor;
    p.offset = d.offset;
    g_panels.push_back(p);
  }
  Serial.printf("[PANELS] %d panel(es) embebidos | gen: %s\n",
                (int)g_panels.size(), DV_PANELS_GEN);
}

// Si hay un panel elegido (y la calib no es 'custom'), re-resuelve su factor/offset
// desde la lista embebida. Llamar en el arranque DESPUES de storeInit + panelsInit.
// Asi un OTA que cambie la calibracion de ese panel se aplica solo en el proximo
// arranque. Si el panel ya no esta en la lista (lo sacamos en un OTA), deja la
// calibracion guardada (no rompe nada).
void panelsApplySelected() {
  if (g_calCustom || g_calPanelB.length() == 0) return;   // manual o sin panel -> respetar lo guardado
  for (auto& p : g_panels) {
    if (p.brand == g_calPanelB && p.model == g_calPanelM) {
      g_factor = p.factor;
      g_offset = p.offset;
      Serial.printf("[PANELS] calib auto: '%s %s' -> f=%.3f o=%.1f\n",
                    g_calPanelB.c_str(), g_calPanelM.c_str(), g_factor, g_offset);
      return;
    }
  }
  Serial.printf("[PANELS] panel '%s %s' ya no esta en la lista; uso calib guardada\n",
                g_calPanelB.c_str(), g_calPanelM.c_str());
}
