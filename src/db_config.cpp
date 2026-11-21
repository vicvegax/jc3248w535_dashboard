#include <WiFi.h>
#include "ui/ui.h"
#include "ui/actions.h"
#include "db_config.h"
#include "config.h"
#include "db_home.h"
#include <Esp.h>
#include <esp_bsp.h>
#include "display.h"
#include "logger.h"
#include <display.h>

namespace {

bool config_ui_criada = false;
lv_obj_t *lb_brilho_val = nullptr;
lv_obj_t *sl_brilho = nullptr;
lv_obj_t *lb_timeout_val = nullptr;
lv_obj_t *bt_timeouts[4] = {nullptr};
lv_obj_t *lb_esp_info = nullptr;
lv_obj_t *lb_wifi_status = nullptr;

const int OPCOES_TIMEOUT[] = {15, 30, 60, 0};

void atualizar_label_timeout(int seg) {
  if (!lb_timeout_val) return;
  if (seg <= 0) {
    lv_label_set_text(lb_timeout_val, "Timeout tela: Nunca");
  } else {
    lv_label_set_text_fmt(lb_timeout_val, "Timeout tela: %ds", seg);
  }
  for (int i = 0; i < 4; i++) {
    if (!bt_timeouts[i]) continue;
    bool ativo = (OPCOES_TIMEOUT[i] == seg);
    lv_obj_set_style_bg_color(
        bt_timeouts[i],
        ativo ? lv_palette_main(LV_PALETTE_BLUE) : lv_color_hex(0x262B36),
        0);
  }
}

void slider_brilho_cb(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);
  lv_obj_t *slider = lv_event_get_target(e);
  int val = lv_slider_get_value(slider);

  if (code == LV_EVENT_VALUE_CHANGED) {
    lv_label_set_text_fmt(lb_brilho_val, "Brilho: %d%%", val);
    bsp_display_brightness_set(val);
  } else if (code == LV_EVENT_RELEASED) {
    salvarBrilhoTela(val);
  }
}

void bt_timeout_cb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
  int seg = (int)(uintptr_t)lv_event_get_user_data(e);
  salvarTimeoutTelaSeg(seg);
  atualizar_label_timeout(seg);
}

void bt_rescan_wifi_cb(lv_event_t *e) {
  if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
    exibir_spinner();
    loopListaWifi = true;
  }
}

void ir_para_home_async(void *param) {
  (void)param;
  lv_tabview_set_act(objects.tv_dashboard, 0, LV_ANIM_OFF);
  lv_info_home();
}

} // namespace

void db_config_init() {
  if (config_ui_criada) return;

  // --- Aba Outras (objects.tab_outras) ---
  lv_obj_t *outras = objects.tab_outras;
  lv_obj_clear_flag(outras, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_pad_all(outras, 8, 0);

  // Reposiciona o botão Resetar ESP32 para o canto inferior direito
  uint32_t cnt = lv_obj_get_child_cnt(outras);
  for (uint32_t i = 0; i < cnt; i++) {
    lv_obj_t *child = lv_obj_get_child(outras, i);
    lv_obj_set_pos(child, 214, 162);
    lv_obj_set_size(child, 134, 42);
    lv_obj_set_style_bg_color(child, lv_palette_main(LV_PALETTE_RED), 0);
  }

  int brilho_atual = obterBrilhoTela();
  lb_brilho_val = lv_label_create(outras);
  lv_label_set_text_fmt(lb_brilho_val, "Brilho: %d%%", brilho_atual);
  lv_obj_set_style_text_font(lb_brilho_val, &lv_font_montserrat_12, 0);
  lv_obj_set_pos(lb_brilho_val, 0, 2);

  sl_brilho = lv_slider_create(outras);
  lv_obj_set_pos(sl_brilho, 0, 24);
  lv_obj_set_size(sl_brilho, 340, 14);
  lv_slider_set_range(sl_brilho, 10, 100);
  lv_slider_set_value(sl_brilho, brilho_atual, LV_ANIM_OFF);
  lv_obj_add_event_cb(sl_brilho, slider_brilho_cb, LV_EVENT_VALUE_CHANGED, NULL);
  lv_obj_add_event_cb(sl_brilho, slider_brilho_cb, LV_EVENT_RELEASED, NULL);

  int timeout_atual = obterTimeoutTelaSeg();
  lb_timeout_val = lv_label_create(outras);
  lv_obj_set_style_text_font(lb_timeout_val, &lv_font_montserrat_12, 0);
  lv_obj_set_pos(lb_timeout_val, 0, 56);
  atualizar_label_timeout(timeout_atual);

  const char *nomes_timeout[] = {"15s", "30s", "60s", "Nunca"};
  for (int i = 0; i < 4; i++) {
    lv_obj_t *btn = lv_btn_create(outras);
    bt_timeouts[i] = btn;
    lv_obj_set_pos(btn, i * 86, 78);
    lv_obj_set_size(btn, 78, 34);
    lv_obj_add_event_cb(btn, bt_timeout_cb, LV_EVENT_CLICKED, (void *)(uintptr_t)OPCOES_TIMEOUT[i]);
    lv_obj_t *lb = lv_label_create(btn);
    lv_label_set_text(lb, nomes_timeout[i]);
    lv_obj_set_style_text_font(lb, &lv_font_montserrat_12, 0);
    lv_obj_center(lb);
  }
  atualizar_label_timeout(timeout_atual);

  lb_esp_info = lv_label_create(outras);
  lv_label_set_text(lb_esp_info, "ESP32: --");
  lv_obj_set_style_text_font(lb_esp_info, &lv_font_montserrat_10, 0);
  lv_obj_set_style_text_color(lb_esp_info, lv_color_hex(0x8B949E), 0);
  lv_obj_set_pos(lb_esp_info, 0, 164);

  // --- Aba Wifi (objects.tab_wifi) ---
  lv_obj_t *wifi_tab = objects.tab_wifi;
  lv_obj_clear_flag(wifi_tab, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_bg_color(objects.bt_esquecer, lv_palette_main(LV_PALETTE_RED), 0);

  lv_obj_t *bt_scan = lv_btn_create(wifi_tab);
  lv_obj_set_pos(bt_scan, 262, -4);
  lv_obj_set_size(bt_scan, 78, 34);
  lv_obj_add_event_cb(bt_scan, bt_rescan_wifi_cb, LV_EVENT_CLICKED, NULL);
  lv_obj_t *lb_scan = lv_label_create(bt_scan);
  lv_label_set_text(lb_scan, LV_SYMBOL_REFRESH " Scan");
  lv_obj_set_style_text_font(lb_scan, &lv_font_montserrat_12, 0);
  lv_obj_center(lb_scan);

  lb_wifi_status = lv_label_create(wifi_tab);
  lv_label_set_text(lb_wifi_status, "");
  lv_obj_set_style_text_font(lb_wifi_status, &lv_font_montserrat_12, 0);
  lv_obj_set_style_text_color(lb_wifi_status, lv_color_hex(0x9AA0A6), 0);
  lv_obj_set_pos(lb_wifi_status, 0, 174);

  config_ui_criada = true;
  db_config_atualizar_info();
}

void db_config_atualizar_info() {
  if (!config_ui_criada) return;

  float v_bat = bateria_ler_tensao();
  int pct_bat = bateria_percentual(v_bat);
  char bat_buf[28];
  snprintf(bat_buf, sizeof(bat_buf), "%.2fV (%d%%)", v_bat, pct_bat);

  if (WiFi.status() == WL_CONNECTED) {
    lv_label_set_text_fmt(lb_esp_info, "IP: %s\nWi-Fi: %s (%d dBm)\nBat: %s | Heap: %u KB",
                          WiFi.localIP().toString().c_str(),
                          WiFi.SSID().c_str(),
                          WiFi.RSSI(),
                          bat_buf,
                          ESP.getFreeHeap() / 1024);
    lv_label_set_text_fmt(lb_wifi_status, "Conectado em %s", WiFi.SSID().c_str());
    lv_obj_set_style_text_color(lb_wifi_status, lv_palette_main(LV_PALETTE_GREEN), 0);
  } else {
    lv_label_set_text_fmt(lb_esp_info, "Wi-Fi: Desconectado\nBat: %s | Heap: %u KB",
                          bat_buf,
                          ESP.getFreeHeap() / 1024);
    lv_label_set_text(lb_wifi_status, "Wi-Fi desconectado");
    lv_obj_set_style_text_color(lb_wifi_status, lv_palette_main(LV_PALETTE_AMBER), 0);
  }
}

extern "C" void action_resetar(lv_event_t *e) {
  (void)e;
  ESP.restart();
}

void escanear_redes() {
  if (!loopListaWifi) return;
  loopListaWifi = false;

  int n = WiFi.scanNetworks(false, false, false, 300);
<<<<<<< HEAD
  String wifi_opcoes = "";
  if (n >= 0) {
    for (int i = 0; i < n; ++i) {
      wifi_opcoes += WiFi.SSID(i) + "\n";
=======
  String opcoes = "";
  if (n > 0) {
    for (int i = 0; i < n; ++i) {
      if (i > 0) opcoes += "\n";
      opcoes += WiFi.SSID(i);
>>>>>>> 4292549 (melhorias:tema,status,alertas,tempo de tela,modal confirmacao)
    }
    WiFi.scanDelete();
    LOG_INFO_1("WIFI", "Localizado %d redes", n);
  } else if (n == 0) {
    opcoes = "Nenhuma rede";
    WiFi.scanDelete();
  } else {
    opcoes = "Erro ao escanear";
    LOG_ERROR_1("WIFI", "Erro ao escanear redes: %d", n);
  }
<<<<<<< HEAD
  
  if(bsp_display_lock(150)) {
    lv_dropdown_set_options(objects.ls_wifi, wifi_opcoes.c_str());
=======

  if (bsp_display_lock(300)) {
    lv_dropdown_set_options(objects.ls_wifi, opcoes.c_str());
>>>>>>> 4292549 (melhorias:tema,status,alertas,tempo de tela,modal confirmacao)
    bsp_display_unlock();
  } else {
    LOG_ERROR("CONFIG", "Erro lock");
  }
  ocultar_spinner();
}

<<<<<<< HEAD
// 1. Crie esta pequena função logo ACIMA da sua action
static void ir_para_home_async(void * param) {
  // Essa função vai rodar com segurança fora do evento de clique!
  lv_obj_t * tab_btns = lv_tabview_get_tab_btns(objects.tv_dashboard);
  
  // Libera os botões com segurança
  lv_btnmatrix_clear_btn_ctrl(tab_btns, 1, LV_BTNMATRIX_CTRL_DISABLED);
  
  // Troca a aba
  lv_tabview_set_act(objects.tv_dashboard, 0, LV_ANIM_OFF);
}

// Ação gerada pelo EEZ Studio ao clicar no botão "OK Conectar"
extern "C" void action_conectar_wifi(lv_event_t * e) {
  lv_obj_t * label = lv_obj_get_child(objects.bt_conectar, 0);
=======
extern "C" void action_conectar_wifi(lv_event_t *e) {
  (void)e;
  lv_obj_t *label = lv_obj_get_child(objects.bt_conectar, 0);
>>>>>>> 4292549 (melhorias:tema,status,alertas,tempo de tela,modal confirmacao)
  lv_label_set_text(label, "Conectando...");

  char ssid_selecionado[64];
  lv_dropdown_get_selected_str(objects.ls_wifi, ssid_selecionado, sizeof(ssid_selecionado));
  const char *senha_digitada = lv_textarea_get_text(objects.ed_wifi_senha);

  if (salvar_e_conectar(String(ssid_selecionado), String(senha_digitada))) {
    db_conectado();
    lv_label_set_text(label, "Conectar");
    db_config_atualizar_info();
    lv_async_call(ir_para_home_async, NULL);
  } else {
    db_desconectado();
    lv_label_set_text(label, "Tentar Nov.");
    if (lb_wifi_status) {
      lv_label_set_text(lb_wifi_status, "Falha ao conectar! Verifique a senha.");
      lv_obj_set_style_text_color(lb_wifi_status, lv_palette_main(LV_PALETTE_RED), 0);
    }
  }
}

static void executar_esquecer_wifi() {
  esquecer_wifi();
  db_desconectado();
  db_config_atualizar_info();

  lv_obj_t *label = lv_obj_get_child(objects.bt_esquecer, 0);
  lv_label_set_text(label, "Esquecido!");
}

extern "C" void action_esquecer_wifi(lv_event_t *e) {
  (void)e;
  exibir_confirmacao("Esquecer Wi-Fi",
                     "Deseja apagar a rede e senha Wi-Fi salvas?",
                     "Esquecer",
                     executar_esquecer_wifi);
}

extern "C" void action_mostrar_senha(lv_event_t *e) {
  lv_obj_t *cb = lv_event_get_target(e);
  lv_textarea_set_password_mode(objects.ed_wifi_senha, !lv_obj_has_state(cb, LV_STATE_CHECKED));
}
<<<<<<< HEAD

extern "C" void action_mudar_brilho(lv_event_t *e) {
  // TODO: Implement action mudar_brilho here
  lv_obj_t * slider = lv_event_get_target(e);

  bsp_display_brightness_set((int)lv_slider_get_value(slider));
}
=======
>>>>>>> 4292549 (melhorias:tema,status,alertas,tempo de tela,modal confirmacao)
