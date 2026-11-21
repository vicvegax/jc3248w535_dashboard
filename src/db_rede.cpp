
#include "ui/ui.h"
#include "db_rede.h"
#include "requisicao.h"
#include "config.h"
#include <esp_bsp.h>
#include <WiFi.h>
#include <cstdlib>

namespace {

constexpr uint32_t REDE_REFRESH_INTERVAL_MS = 5000;
constexpr uint32_t REDE_CHECK_INTERVAL_MS = 500;

bool rede_ui_criada = false;
uint32_t ultimo_tick_rede_ms = 0;
uint32_t ultimo_request_rede_ms = 0;

lv_obj_t *lb_rede_status = nullptr;
lv_obj_t *lb_dl_val = nullptr;
lv_obj_t *lb_ul_val = nullptr;
lv_obj_t *lb_ping_val = nullptr;
lv_obj_t *lb_ping_qual = nullptr;
lv_obj_t *bar_ping = nullptr;
lv_obj_t *lb_rede_host = nullptr;

lv_color_t cor_por_ping(float ping_ms) {
  if (ping_ms >= 80.0f) return lv_palette_main(LV_PALETTE_RED);
  if (ping_ms >= 30.0f) return lv_palette_main(LV_PALETTE_AMBER);
  return lv_palette_main(LV_PALETTE_GREEN);
}

const char *texto_qualidade_ping(float ping_ms) {
  if (ping_ms >= 80.0f) return "Alto";
  if (ping_ms >= 30.0f) return "Moderado";
  return "Excelente";
}

lv_obj_t *criar_card(lv_obj_t *parent, lv_coord_t x, lv_coord_t y, lv_coord_t w, lv_coord_t h) {
  lv_obj_t *card = lv_obj_create(parent);
  lv_obj_set_pos(card, x, y);
  lv_obj_set_size(card, w, h);
  lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_bg_color(card, lv_color_hex(0x1E222A), 0);
  lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
  lv_obj_set_style_border_color(card, lv_color_hex(0x323846), 0);
  lv_obj_set_style_border_width(card, 1, 0);
  lv_obj_set_style_radius(card, 8, 0);
  lv_obj_set_style_pad_all(card, 10, 0);
  return card;
}

void lv_info_rede_cb(JsonDocument &doc) {
  if (!rede_ui_criada) return;

  const char *dl = doc["download"] | "--";
  const char *ul = doc["upload"] | "--";

  lv_label_set_text(lb_dl_val, dl);
  lv_label_set_text(lb_ul_val, ul);

  if (doc["ping_ms"].isNull()) {
    lv_label_set_text(lb_ping_val, "NA");
    lv_obj_set_style_text_color(lb_ping_val, lv_color_hex(0x9AA0A6), 0);
    lv_label_set_text(lb_ping_qual, "Indisponivel");
    lv_obj_set_style_text_color(lb_ping_qual, lv_color_hex(0x9AA0A6), 0);
    lv_bar_set_value(bar_ping, 0, LV_ANIM_OFF);
  } else {
    float ping_ms = doc["ping_ms"].as<float>();
    lv_color_t cor = cor_por_ping(ping_ms);

    char ping_buf[24];
    snprintf(ping_buf, sizeof(ping_buf), "%.1f ms", ping_ms);
    lv_label_set_text(lb_ping_val, ping_buf);
    lv_obj_set_style_text_color(lb_ping_val, cor, 0);
    lv_label_set_text(lb_ping_qual, texto_qualidade_ping(ping_ms));
    lv_obj_set_style_text_color(lb_ping_qual, cor, 0);

    int bar_val = static_cast<int>(ping_ms + 0.5f);
    if (bar_val < 0) bar_val = 0;
    if (bar_val > 150) bar_val = 150;
    lv_bar_set_value(bar_ping, bar_val, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(bar_ping, cor, LV_PART_INDICATOR);
  }

  lv_label_set_text_fmt(lb_rede_host, "No: %s", cluster.http.c_str());
  lv_label_set_text(lb_rede_status, "Online");
  lv_obj_set_style_text_color(lb_rede_status, lv_palette_main(LV_PALETTE_GREEN), 0);
}

void lv_info_rede_erro_cb(int httpCode) {
  if (!rede_ui_criada) return;
  if (httpCode == -100) {
    lv_label_set_text(lb_rede_status, "Sem Wi-Fi");
  } else if (httpCode > 0) {
    lv_label_set_text_fmt(lb_rede_status, "HTTP %d", httpCode);
  } else {
    lv_label_set_text(lb_rede_status, "Offline");
  }
  lv_obj_set_style_text_color(lb_rede_status, lv_palette_main(LV_PALETTE_RED), 0);
}

void bt_rede_refresh_cb(lv_event_t *e) {
  if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
    lv_info_rede();
  }
}

} // namespace

void db_rede_init() {
  if (rede_ui_criada) return;

  lv_obj_t *tab = objects.tab_rede;
  lv_obj_clear_flag(tab, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_pad_all(tab, 8, 0);

  // Oculta os labels legados do EEZ Studio
  lv_obj_add_flag(objects.lb_download, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(objects.lb_upload, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(objects.lb_ping, LV_OBJ_FLAG_HIDDEN);

  // Cabeçalho
  lv_obj_t *lb_titulo = lv_label_create(tab);
  lv_label_set_text(lb_titulo, "Rede");
  lv_obj_set_style_text_font(lb_titulo, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(lb_titulo, lv_color_hex(0xD0D7DE), 0);
  lv_obj_set_pos(lb_titulo, 0, 4);

  lb_rede_status = lv_label_create(tab);
  lv_label_set_text(lb_rede_status, "Aguardando...");
  lv_obj_set_style_text_font(lb_rede_status, &lv_font_montserrat_12, 0);
  lv_obj_set_style_text_color(lb_rede_status, lv_color_hex(0x8B949E), 0);
  lv_obj_set_pos(lb_rede_status, 52, 6);

  lv_obj_t *bt_refresh = lv_btn_create(tab);
  lv_obj_set_size(bt_refresh, 92, 26);
  lv_obj_set_pos(bt_refresh, 282, 0);
  lv_obj_add_event_cb(bt_refresh, bt_rede_refresh_cb, LV_EVENT_CLICKED, NULL);
  lv_obj_t *lb_ref = lv_label_create(bt_refresh);
  lv_label_set_text(lb_ref, LV_SYMBOL_REFRESH " Atual.");
  lv_obj_set_style_text_font(lb_ref, &lv_font_montserrat_12, 0);
  lv_obj_center(lb_ref);

  // Cards Download e Upload
  constexpr lv_coord_t CARD_W = 182;
  constexpr lv_coord_t CARD_H = 124;

  lv_obj_t *card_dl = criar_card(tab, 0, 36, CARD_W, CARD_H);
  lv_obj_t *lb_dl_title = lv_label_create(card_dl);
  lv_label_set_text(lb_dl_title, LV_SYMBOL_DOWNLOAD " Download");
  lv_obj_set_style_text_font(lb_dl_title, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(lb_dl_title, lv_palette_main(LV_PALETTE_LIGHT_BLUE), 0);
  lv_obj_align(lb_dl_title, LV_ALIGN_TOP_LEFT, 0, 0);

  lb_dl_val = lv_label_create(card_dl);
  lv_label_set_text(lb_dl_val, "-- MB");
  lv_label_set_long_mode(lb_dl_val, LV_LABEL_LONG_DOT);
  lv_obj_set_width(lb_dl_val, 160);
  lv_obj_set_style_text_font(lb_dl_val, &lv_font_montserrat_22, 0);
  lv_obj_set_style_text_color(lb_dl_val, lv_color_white(), 0);
  lv_obj_align(lb_dl_val, LV_ALIGN_CENTER, 0, 4);

  lv_obj_t *lb_dl_sub = lv_label_create(card_dl);
  lv_label_set_text(lb_dl_sub, "Total recebido");
  lv_obj_set_style_text_font(lb_dl_sub, &lv_font_montserrat_10, 0);
  lv_obj_set_style_text_color(lb_dl_sub, lv_color_hex(0x8B949E), 0);
  lv_obj_align(lb_dl_sub, LV_ALIGN_BOTTOM_LEFT, 0, 0);

  lv_obj_t *card_ul = criar_card(tab, CARD_W + 10, 36, CARD_W, CARD_H);
  lv_obj_t *lb_ul_title = lv_label_create(card_ul);
  lv_label_set_text(lb_ul_title, LV_SYMBOL_UPLOAD " Upload");
  lv_obj_set_style_text_font(lb_ul_title, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(lb_ul_title, lv_palette_main(LV_PALETTE_TEAL), 0);
  lv_obj_align(lb_ul_title, LV_ALIGN_TOP_LEFT, 0, 0);

  lb_ul_val = lv_label_create(card_ul);
  lv_label_set_text(lb_ul_val, "-- MB");
  lv_label_set_long_mode(lb_ul_val, LV_LABEL_LONG_DOT);
  lv_obj_set_width(lb_ul_val, 160);
  lv_obj_set_style_text_font(lb_ul_val, &lv_font_montserrat_22, 0);
  lv_obj_set_style_text_color(lb_ul_val, lv_color_white(), 0);
  lv_obj_align(lb_ul_val, LV_ALIGN_CENTER, 0, 4);

  lv_obj_t *lb_ul_sub = lv_label_create(card_ul);
  lv_label_set_text(lb_ul_sub, "Total enviado");
  lv_obj_set_style_text_font(lb_ul_sub, &lv_font_montserrat_10, 0);
  lv_obj_set_style_text_color(lb_ul_sub, lv_color_hex(0x8B949E), 0);
  lv_obj_align(lb_ul_sub, LV_ALIGN_BOTTOM_LEFT, 0, 0);

  // Card Inferior: Latência / Ping
  lv_obj_t *card_ping = criar_card(tab, 0, 170, 374, 98);

  lv_obj_t *lb_ping_title = lv_label_create(card_ping);
  lv_label_set_text(lb_ping_title, "Latencia (Ping)");
  lv_obj_set_style_text_font(lb_ping_title, &lv_font_montserrat_12, 0);
  lv_obj_set_style_text_color(lb_ping_title, lv_color_hex(0x8B949E), 0);
  lv_obj_align(lb_ping_title, LV_ALIGN_TOP_LEFT, 0, 0);

  lb_ping_qual = lv_label_create(card_ping);
  lv_label_set_text(lb_ping_qual, "--");
  lv_obj_set_style_text_font(lb_ping_qual, &lv_font_montserrat_12, 0);
  lv_obj_set_style_text_color(lb_ping_qual, lv_color_hex(0x8B949E), 0);
  lv_obj_align(lb_ping_qual, LV_ALIGN_TOP_RIGHT, 0, 0);

  lb_ping_val = lv_label_create(card_ping);
  lv_label_set_text(lb_ping_val, "-- ms");
  lv_obj_set_style_text_font(lb_ping_val, &lv_font_montserrat_22, 0);
  lv_obj_set_style_text_color(lb_ping_val, lv_color_white(), 0);
  lv_obj_set_pos(lb_ping_val, 0, 22);

  lb_rede_host = lv_label_create(card_ping);
  lv_label_set_text(lb_rede_host, "No: --");
  lv_obj_set_style_text_font(lb_rede_host, &lv_font_montserrat_10, 0);
  lv_obj_set_style_text_color(lb_rede_host, lv_color_hex(0x8B949E), 0);
  lv_obj_align(lb_rede_host, LV_ALIGN_RIGHT_MID, 0, -4);

  bar_ping = lv_bar_create(card_ping);
  lv_obj_set_size(bar_ping, 352, 10);
  lv_bar_set_range(bar_ping, 0, 150);
  lv_bar_set_value(bar_ping, 0, LV_ANIM_OFF);
  lv_obj_set_style_bg_color(bar_ping, lv_color_hex(0x2D3340), LV_PART_MAIN);
  lv_obj_set_style_bg_color(bar_ping, lv_palette_main(LV_PALETTE_GREEN), LV_PART_INDICATOR);
  lv_obj_align(bar_ping, LV_ALIGN_BOTTOM_MID, 0, -2);

  rede_ui_criada = true;
}

void lv_info_rede(bool show_spinner) {
  if (!rede_ui_criada) {
    db_rede_init();
  }

  if (cluster.http == "") {
    lv_label_set_text(lb_rede_status, "Sem cluster");
    lv_obj_set_style_text_color(lb_rede_status, lv_palette_main(LV_PALETTE_AMBER), 0);
    return;
  }

  ultimo_request_rede_ms = millis();
  if (show_spinner) {
    lv_label_set_text(lb_rede_status, "Atualizando...");
    lv_obj_set_style_text_color(lb_rede_status, lv_color_hex(0x9AA0A6), 0);
  }
  get("/network", lv_info_rede_cb, show_spinner, lv_info_rede_erro_cb);
}

void db_rede_tick() {
  if (!rede_ui_criada) return;
  if (WiFi.status() != WL_CONNECTED || cluster.http == "") return;

  uint32_t agora = millis();
  if (agora - ultimo_tick_rede_ms < REDE_CHECK_INTERVAL_MS) return;
  ultimo_tick_rede_ms = agora;

  bool rede_ativa = false;
  if (bsp_display_lock(30)) {
    rede_ativa = (lv_scr_act() == objects.main) &&
                 (lv_tabview_get_tab_act(objects.tv_dashboard) == 3);
    bsp_display_unlock();
  }
  if (!rede_ativa) return;

  if (!req_atual.pendente && (agora - ultimo_request_rede_ms >= REDE_REFRESH_INTERVAL_MS)) {
    lv_info_rede(false);
  }
}
