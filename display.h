/*  display.h — LovyanGFX (ST7789 + CST816) + glue de LVGL 8.4  */
#pragma once
#define LGFX_USE_V1
#include <LovyanGFX.hpp>
#include <lvgl.h>
#include "esp_timer.h"
#include "esp_heap_caps.h"
#include "dv_pins.h"

// ───────────────────────── LovyanGFX ─────────────────────────
class LGFX : public lgfx::LGFX_Device {
  lgfx::Panel_ST7789  _panel;
  lgfx::Bus_SPI       _bus;
  lgfx::Light_PWM     _light;
  lgfx::Touch_CST816S _touch;
public:
  LGFX() {
    { auto c = _bus.config();
      c.spi_host    = SPI2_HOST;
      c.spi_mode    = 0;
      c.freq_write  = 40000000;
      c.freq_read   = 16000000;
      c.spi_3wire   = false;
      c.use_lock    = true;
      c.dma_channel = SPI_DMA_CH_AUTO;
      c.pin_sclk    = PIN_LCD_SCLK;
      c.pin_mosi    = PIN_LCD_MOSI;
      c.pin_miso    = PIN_LCD_MISO;
      c.pin_dc      = PIN_LCD_DC;
      _bus.config(c);
      _panel.setBus(&_bus);
    }
    { auto c = _panel.config();
      c.pin_cs    = PIN_LCD_CS;
      c.pin_rst   = PIN_LCD_RST;
      c.pin_busy  = -1;
      c.panel_width      = SCR_W;
      c.panel_height     = SCR_H;
      c.offset_x         = 0;
      c.offset_y         = 0;
      c.offset_rotation  = 0;
      c.dummy_read_pixel = 8;
      c.dummy_read_bits  = 1;
      c.readable   = false;
      c.invert     = true;     // IPS ST7789
      c.rgb_order  = false;
      c.dlen_16bit = false;
      c.bus_shared = false;
      _panel.config(c);
    }
    { auto c = _light.config();
      c.pin_bl      = PIN_LCD_BL;
      c.invert      = false;
      c.freq        = 44100;
      c.pwm_channel = 7;
      _light.config(c);
      _panel.setLight(&_light);
    }
    { auto c = _touch.config();
      c.x_min = 0;  c.x_max = SCR_W - 1;
      c.y_min = 0;  c.y_max = SCR_H - 1;
      c.pin_int  = PIN_TP_INT;
      c.pin_rst  = -1;
      c.bus_shared = false;
      c.offset_rotation = 0;
      c.i2c_port = 0;
      c.i2c_addr = 0x15;
      c.pin_sda  = PIN_TP_SDA;
      c.pin_scl  = PIN_TP_SCL;
      c.freq     = 400000;
      _touch.config(c);
      _panel.setTouch(&_touch);
    }
    setPanel(&_panel);
  }
};
LGFX lcd;

// Brillo del backlight + handle del tick de LVGL (los usa el ahorro de energia)
#define DV_BL_FULL  200             // brillo normal (100%)
#define DV_BL_SAVE  (DV_BL_FULL/2)  // brillo en modo ahorro (50%)
#define DV_BL_CRIT  12              // brillo minimo (aviso de bateria critica al arrancar)
static uint8_t          dv_blAwake   = DV_BL_FULL;  // brillo con la pantalla encendida
static esp_timer_handle_t dv_tickTimer = nullptr;

// ───────────────────────── Glue LVGL ─────────────────────────
static lv_disp_draw_buf_t dv_draw_buf;
static lv_color_t* dv_buf1 = nullptr;
static lv_color_t* dv_buf2 = nullptr;
static lv_disp_drv_t dv_disp_drv;          // file-scope: permite rotar el display en runtime
static lv_disp_t*    dv_disp = nullptr;    // handle del display registrado

static void dv_disp_flush(lv_disp_drv_t* drv, const lv_area_t* area, lv_color_t* px) {
  uint32_t w = area->x2 - area->x1 + 1;
  uint32_t h = area->y2 - area->y1 + 1;
  lcd.startWrite();
  lcd.setAddrWindow(area->x1, area->y1, w, h);
#if LV_COLOR_16_SWAP
  lcd.writePixels((lgfx::swap565_t*)px, w * h);
#else
  lcd.writePixels((lgfx::rgb565_t*)px, w * h);
#endif
  lcd.endWrite();
  lv_disp_flush_ready(drv);
}

static void dv_touch_read(lv_indev_drv_t* drv, lv_indev_data_t* data) {
  uint16_t x, y;
  if (lcd.getTouch(&x, &y)) {
    data->state   = LV_INDEV_STATE_PR;
    data->point.x = x;
    data->point.y = y;
  } else {
    data->state = LV_INDEV_STATE_REL;
  }
}

#if !LV_TICK_CUSTOM
static void dv_lv_tick_cb(void*) { lv_tick_inc(1); }
#endif

// Inicializa pantalla + LVGL. Muestra un splash breve.
void displayInit() {
  lcd.init();
  lcd.setRotation(0);
  lcd.setBrightness(dv_blAwake);

  // splash
  lcd.fillScreen(lcd.color565(10, 15, 30));
  lcd.setTextColor(lcd.color565(0, 204, 255));
  lcd.setTextDatum(textdatum_t::middle_center);
  lcd.setTextSize(2);
  lcd.drawString("DRUIDA VISION", SCR_W / 2, SCR_H / 2);
  delay(700);

  lv_init();
  const uint32_t bufPx = SCR_W * 40;
  dv_buf1 = (lv_color_t*)heap_caps_malloc(bufPx * sizeof(lv_color_t), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  dv_buf2 = (lv_color_t*)heap_caps_malloc(bufPx * sizeof(lv_color_t), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  if (!dv_buf1) {
    dv_buf1 = (lv_color_t*)heap_caps_malloc(bufPx * sizeof(lv_color_t), MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    dv_buf2 = nullptr;
  }
  lv_disp_draw_buf_init(&dv_draw_buf, dv_buf1, dv_buf2, bufPx);

  lv_disp_drv_init(&dv_disp_drv);
  dv_disp_drv.hor_res  = SCR_W;
  dv_disp_drv.ver_res  = SCR_H;
  dv_disp_drv.flush_cb = dv_disp_flush;
  dv_disp_drv.draw_buf = &dv_draw_buf;
  dv_disp = lv_disp_drv_register(&dv_disp_drv);

  static lv_indev_drv_t id;
  lv_indev_drv_init(&id);
  id.type    = LV_INDEV_TYPE_POINTER;
  id.read_cb = dv_touch_read;
  lv_indev_drv_register(&id);

#if !LV_TICK_CUSTOM
  esp_timer_create_args_t ta = {};
  ta.callback = &dv_lv_tick_cb;
  ta.name     = "lv_tick";
  esp_timer_create(&ta, &dv_tickTimer);
  esp_timer_start_periodic(dv_tickTimer, 1000);
#endif
}

// ── Helpers para el ahorro de energia (los usa power.h / ui.h) ──
// Enciende/apaga el backlight (el mayor consumo de la placa).
void displayBacklight(bool on) { lcd.setBrightness(on ? dv_blAwake : 0); }
// Fija el brillo "despierto": 50% en modo ahorro, 100% normal. Lo aplica ya
// (se llama con la pantalla encendida, desde el switch de Config o el boot).
void displaySetSaver(bool saver) {
  dv_blAwake = saver ? DV_BL_SAVE : DV_BL_FULL;
  lcd.setBrightness(dv_blAwake);
}
// Pausa/reanuda el tick de 1 ms (debe estar detenido durante el light-sleep,
// si no su interrupcion despertaria al micro de inmediato).
void displayTickPause()  { if (dv_tickTimer) esp_timer_stop(dv_tickTimer); }
void displayTickResume() { if (dv_tickTimer) esp_timer_start_periodic(dv_tickTimer, 1000); }

// Rota el panel + LVGL. false = vertical (240x320, por defecto); true = horizontal
// (320x240). LovyanGFX rota pantalla y touch juntos, asi que el touch sigue calibrado.
// Lo usa el panel de ingreso de WiFi en Config para mostrar un teclado mas ancho.
void displaySetRotation(bool landscape) {
  lcd.setRotation(landscape ? 1 : 0);
  dv_disp_drv.hor_res = landscape ? SCR_H : SCR_W;   // 320 / 240
  dv_disp_drv.ver_res = landscape ? SCR_W : SCR_H;   // 240 / 320
  lv_disp_drv_update(dv_disp, &dv_disp_drv);
}
