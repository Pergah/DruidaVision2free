/*  panels_data.h — LISTA DE CALIBRACIONES (GENERADO — NO EDITAR A MANO).
 *
 *  Generado por la webApp en /admin/paneles-led ("Exportar .h" o "Subir a GitHub").
 *  Se sobrescribe COMPLETO en cada release: para cambiar calibraciones,
 *  editalas en la web y volve a generar este archivo.
 *
 *  Depende de struct DvPanelDef (definido en panels.h, que incluye este archivo).
 *
 *  Release (firmware + calibraciones en un solo OTA):
 *    1. Reemplazar este archivo en el proyecto DruidaVision2.
 *    2. Arduino IDE: Export Compiled Binary + push_druidavision.bat.
 */
#pragma once

#define DV_PANELS_GEN  "2026-06-14 03:11:11 - 3 panel(es)"

static const DvPanelDef DV_PANELS_LIST[] = {
  { "Factor Neutro", "Factor Neutro", 1.0f, 0.0f },
  { "Lux Horticultura", "Spyder Normal", 1.26f, 0.0f },
  { "Lux Horticultura", "Spyder Pro", 1.4022f, 0.0f },
};
