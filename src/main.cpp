
#include <Arduino.h>
#include <lvgl.h>
#include "display.h"
#include "esp_bsp.h"
#include "lv_port.h"
#include "ui/ui.h"
#include "ui/screens.h"
#include "ui/actions.h"
#include "config.h"
#include "teclado.h"
#include "db_config.h"
#include "db_home.h"
#include "db_rede.h"
#include "db_cluster.h"
#include "requisicao.h"
#include "db_docker.h"
#include "db_alertas.h"
#include "logger.h"
#include <Battery.h>

// Exemplo: Pino A4, com divisor resistivo genérico do JC3248 (ajuste o 2.0f se o hardware usar resistores diferentes, ex: 33k+100k/100k = 1.33)
#define BATTERY_PIN         A4

Battery battery(3100, 4100, BATTERY_PIN);

#define DEBUG_TASKS() \
  Serial.printf("Stack livre Loop: %d | Stack livre GUI: %d\n", \
  uxTaskGetStackHighWaterMark(NULL), \
  uxTaskGetStackHighWaterMark(xTaskGetHandle("LVGL task"))) // ou o nome que o bsp usa


#define BLINK_LED
#define LED_PIN         (17)

char bateria[8];

uint32_t ultima_requisicao;
void (*requisicao)() = nullptr;

uint32_t ultimo_heartbeat;
static bool tela_apagada = false;

// Tempo de inatividade em milissegundos (ex: 30 segundos)
#define TEMPO_INATIVIDADE_MS (30000)

uint8_t tempoRequisicao = 5; //em segundos
uint8_t tempoScreensaver = 60;

lv_timer_t *timerRequisicao;

lv_style_t estilo_checked;
static uint16_t ultima_aba_aberta = -1;
static void focus_tab(lv_obj_t *tabview, lv_obj_t *target_page, bool send_event = true);

<<<<<<< HEAD
uint32_t contador = 0;
=======
static constexpr uint32_t BACKLIGHT_CHECK_INTERVAL_MS = 250;
static constexpr uint32_t BATTERY_CHECK_INTERVAL_MS = 5000;

static volatile bool backlight_is_on = true;
static lv_obj_t *sleep_overlay = nullptr;
static uint32_t last_backlight_check_ms = 0;
static uint32_t last_battery_check_ms = 0;
static lv_obj_t *lb_bateria = nullptr;

static void sleep_overlay_cb(lv_event_t *e) {
  if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
    backlight_is_on = true;
    bsp_display_brightness_set(obterBrilhoTela());
    if (sleep_overlay) {
      lv_obj_add_flag(sleep_overlay, LV_OBJ_FLAG_HIDDEN);
    }
    lv_disp_trig_activity(NULL);
  }
}

static void atualizar_label_bateria() {
  if (!lb_bateria) return;
  float v = bateria_ler_tensao();
  int pct = bateria_percentual(v);

  const char *icone = LV_SYMBOL_BATTERY_EMPTY;
  lv_color_t cor = lv_palette_main(LV_PALETTE_RED);
  if (pct >= 85) {
    icone = LV_SYMBOL_BATTERY_FULL;
    cor = lv_palette_main(LV_PALETTE_GREEN);
  } else if (pct >= 60) {
    icone = LV_SYMBOL_BATTERY_3;
    cor = lv_palette_main(LV_PALETTE_GREEN);
  } else if (pct >= 35) {
    icone = LV_SYMBOL_BATTERY_2;
    cor = lv_palette_main(LV_PALETTE_AMBER);
  } else if (pct >= 15) {
    icone = LV_SYMBOL_BATTERY_1;
    cor = lv_palette_main(LV_PALETTE_AMBER);
  }

  lv_label_set_text_fmt(lb_bateria, "%s %d%%", icone, pct);
  lv_obj_set_style_text_color(lb_bateria, cor, 0);
}

static void aplicar_estilo_input_escuro(lv_obj_t *obj) {
  lv_obj_set_style_bg_color(obj, lv_color_hex(0x1E222A), 0);
  lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
  lv_obj_set_style_border_color(obj, lv_color_hex(0x323846), 0);
  lv_obj_set_style_border_width(obj, 1, 0);
  lv_obj_set_style_text_color(obj, lv_color_white(), 0);
}

static void aplicar_tema_escuro() {
  // Ativa modo escuro no tema padrão do LVGL para todos os widgets criados em seguida
  lv_disp_t *dispp = lv_disp_get_default();
  lv_theme_t *theme = lv_theme_default_init(
      dispp,
      lv_palette_main(LV_PALETTE_BLUE),
      lv_palette_main(LV_PALETTE_RED),
      true,
      LV_FONT_DEFAULT);
  lv_disp_set_theme(dispp, theme);

  // Telas base
  lv_obj_set_style_bg_color(objects.main, lv_color_hex(0x13161C), 0);
  lv_obj_set_style_text_color(objects.main, lv_color_hex(0xD0D7DE), 0);
  lv_obj_set_style_bg_color(objects.form_cluster, lv_color_hex(0x13161C), 0);
  lv_obj_set_style_text_color(objects.form_cluster, lv_color_hex(0xD0D7DE), 0);

  // Barra superior
  lv_obj_set_style_text_color(objects.lb_conectado, lv_color_hex(0xD0D7DE), 0);
  lv_obj_set_style_text_color(objects.lb_cluster, lv_color_hex(0x8B949E), 0);

  // TabView Principal (barra lateral esquerda)
  lv_obj_set_style_bg_color(objects.tv_dashboard, lv_color_hex(0x13161C), 0);
  lv_obj_t *tab_btns = lv_tabview_get_tab_btns(objects.tv_dashboard);
  lv_obj_set_style_bg_color(tab_btns, lv_color_hex(0x181B22), 0);
  lv_obj_set_style_border_color(tab_btns, lv_color_hex(0x2A303C), 0);
  lv_obj_set_style_border_width(tab_btns, 1, 0);
  lv_obj_set_style_border_side(tab_btns, LV_BORDER_SIDE_RIGHT, 0);
  lv_obj_set_style_text_color(tab_btns, lv_color_hex(0x8B949E), LV_PART_ITEMS);
  lv_obj_set_style_bg_color(tab_btns, lv_color_hex(0x1E222A), LV_PART_ITEMS | LV_STATE_CHECKED);
  lv_obj_set_style_text_color(tab_btns, lv_color_white(), LV_PART_ITEMS | LV_STATE_CHECKED);
  lv_obj_set_style_border_color(tab_btns, lv_palette_main(LV_PALETTE_BLUE), LV_PART_ITEMS | LV_STATE_CHECKED);
  lv_obj_set_style_border_width(tab_btns, 3, LV_PART_ITEMS | LV_STATE_CHECKED);
  lv_obj_set_style_border_side(tab_btns, LV_BORDER_SIDE_RIGHT, LV_PART_ITEMS | LV_STATE_CHECKED);

  // TabView Config (barra superior)
  lv_obj_set_style_bg_color(objects.tv_config, lv_color_hex(0x13161C), 0);
  lv_obj_t *cfg_btns = lv_tabview_get_tab_btns(objects.tv_config);
  lv_obj_set_style_bg_color(cfg_btns, lv_color_hex(0x181B22), 0);
  lv_obj_set_style_text_color(cfg_btns, lv_color_hex(0x8B949E), LV_PART_ITEMS);
  lv_obj_set_style_bg_color(cfg_btns, lv_color_hex(0x1E222A), LV_PART_ITEMS | LV_STATE_CHECKED);
  lv_obj_set_style_text_color(cfg_btns, lv_color_white(), LV_PART_ITEMS | LV_STATE_CHECKED);
  lv_obj_set_style_border_color(cfg_btns, lv_palette_main(LV_PALETTE_BLUE), LV_PART_ITEMS | LV_STATE_CHECKED);
  lv_obj_set_style_border_width(cfg_btns, 2, LV_PART_ITEMS | LV_STATE_CHECKED);
  lv_obj_set_style_border_side(cfg_btns, LV_BORDER_SIDE_BOTTOM, LV_PART_ITEMS | LV_STATE_CHECKED);

  // Listas (Cluster e Docker)
  aplicar_estilo_input_escuro(objects.ls_clusters);
  lv_obj_set_style_bg_color(objects.ls_clusters, lv_color_hex(0x181B22), 0);
  lv_obj_set_style_radius(objects.ls_clusters, 8, 0);

  aplicar_estilo_input_escuro(objects.ls_cont);
  lv_obj_set_style_bg_color(objects.ls_cont, lv_color_hex(0x181B22), 0);
  lv_obj_set_style_radius(objects.ls_cont, 8, 0);

  // Inputs (Wi-Fi e Form Cluster)
  aplicar_estilo_input_escuro(objects.ls_wifi);
  aplicar_estilo_input_escuro(objects.ed_wifi_senha);
  aplicar_estilo_input_escuro(objects.ed_cluster_nome);
  aplicar_estilo_input_escuro(objects.ed_cluster_http);
  aplicar_estilo_input_escuro(objects.ed_cluster_port);
  lv_obj_set_style_text_color(objects.ch_ver_senha, lv_color_hex(0xD0D7DE), 0);

  // Teclado e Spinner
  lv_obj_set_style_bg_color(objects.keyboard_1, lv_color_hex(0x181B22), 0);
  lv_obj_set_style_bg_color(objects.keyboard_1, lv_color_hex(0x262B36), LV_PART_ITEMS);
  lv_obj_set_style_text_color(objects.keyboard_1, lv_color_white(), LV_PART_ITEMS);
  lv_obj_set_style_bg_color(objects.pn_spinner, lv_color_hex(0x0D0F14), 0);
}
>>>>>>> 4292549 (melhorias:tema,status,alertas,tempo de tela,modal confirmacao)

void cb_log(const char* buf) {
  Serial.println("****GERADO PELO CALLBACK****");
  Serial.println(buf);
  Serial.println("****************************");
}

// Timer do LVGL para atualizar a UI a cada 1 segundo
void battery_ui_timer_cb(lv_timer_t * timer) {
  // Variáveis estáticas guardam o último estado desenhado
  static uint8_t last_pct = 255; 
  static bool last_charging = false;

  uint8_t current_pct = battery.level();// BatteryMonitor::getInstance().getPercentage();
  bool current_charging = false;
  lv_label_set_text_fmt(objects.lb_bateriav, "%d", battery.voltage());
  // SÓ processa o LVGL se algo realmente mudou!
  if ((current_pct != last_pct || current_charging != last_charging) && !tela_apagada) {
    
    if (current_charging) {
      lv_label_set_text_fmt(objects.lb_bateria, LV_SYMBOL_CHARGE " %d%%", current_pct);
    } else {
      const char* icon = LV_SYMBOL_BATTERY_FULL;
      if (current_pct <= 20) icon = LV_SYMBOL_BATTERY_EMPTY;
      else if (current_pct <= 50) icon = LV_SYMBOL_BATTERY_2;
      else if (current_pct <= 80) icon = LV_SYMBOL_BATTERY_3;

      lv_label_set_text_fmt(objects.lb_bateria, "%s %d%%", icon, current_pct);
    }

    // Atualiza a memória
    last_pct = current_pct;
    last_charging = current_charging;
  }
}

extern "C" void action_close_screensaver(lv_event_t *e) {
  lv_scr_load(objects.main);
  bsp_display_brightness_set(25); 
  tela_apagada = false;
  lv_timer_ready(timerRequisicao);
}

void timer_requisicao(lv_timer_t * timer) {
  if(requisicao && !tela_apagada) {
    LOG_INFO("rodando do timer", "REQUISICAO");
    requisicao();
  } else {
    if(!tela_apagada) {
      LOG_WARN("rodando do timer", "NAO TEM REQUISICAO!");
    }
  }
  contador++;
  lv_label_set_text(objects.lb_contador, String(contador).c_str());
  lv_timer_reset(timer);
};

//  SSS  EEEE TTTTTT U   U PPPP  
// S     E      TT   U   U P   P 
//  SSS  EEE    TT   U   U PPPP  
//     S E      TT   U   U P     
// SSSS  EEEE   TT    UUU  P     
void setup() {
  //(68k+100k)/100k
	battery.begin(1100, float(33+100)/(float)100,  &sigmoidal);

  String title = "Dashboard";

  Serial.begin(115200);
  LOG_INFO(title, "INICIALIZANDO...");

  lv_log_register_print_cb(cb_log);
  analogSetPinAttenuation(BATTERY_PIN, ADC_11db);
  //task_affinity = 1 <<<<<<<<< FIXA NO CORE 1 (LOOP)
  bsp_display_cfg_t cfg = {
    .lvgl_port_cfg = {
      .task_priority = 4,
      .task_stack = 16384,
      .task_affinity = 1, //FIXADO NO CORE 1 (LOOP)
      .task_max_sleep_ms = 500,
      .timer_period_ms = 5,
    },
    .buffer_size = EXAMPLE_LCD_QSPI_H_RES * EXAMPLE_LCD_QSPI_V_RES,
    .rotate = LV_DISP_ROT_90,
  };

  bsp_display_start_with_config(&cfg);
<<<<<<< HEAD
  // Inicia com brilho máximo (255)
=======
  bsp_display_backlight_on();
  bsp_display_brightness_set(obterBrilhoTela());
>>>>>>> 4292549 (melhorias:tema,status,alertas,tempo de tela,modal confirmacao)

  LOG_INFO(title, "Criando UI...");
  /* Lock the mutex due to the LVGL APIs are not thread-safe */
  bsp_display_lock(0); //SETUP

  ui_init();
  aplicar_tema_escuro();
  bateria_init();

  lb_bateria = lv_label_create(objects.main);
  lv_obj_set_pos(lb_bateria, 175, 9);
  lv_obj_set_style_text_font(lb_bateria, &lv_font_montserrat_12, 0);
  atualizar_label_bateria();

  db_home_init();
  db_cluster_init();
  db_docker_init();
  db_rede_init();
  db_alertas_init();
  db_config_init();

  lv_style_init(&estilo_checked);
  // Cor de fundo azul quando marcado
  lv_style_set_bg_color(&estilo_checked, lv_palette_main(LV_PALETTE_BLUE)); 
  lv_style_set_bg_opa(&estilo_checked, LV_OPA_COVER);
  // Texto branco quando marcado
  lv_style_set_text_color(&estilo_checked, lv_color_white()); 

  lv_obj_set_parent(objects.keyboard_1, lv_layer_top());
  lv_obj_add_flag(objects.keyboard_1, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(objects.pn_spinner, LV_OBJ_FLAG_HIDDEN);

  // Overlay transparente para absorver o primeiro toque ao acordar a tela
  sleep_overlay = lv_obj_create(lv_layer_top());
  lv_obj_set_size(sleep_overlay, lv_pct(100), lv_pct(100));
  lv_obj_set_pos(sleep_overlay, 0, 0);
  lv_obj_clear_flag(sleep_overlay, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(sleep_overlay, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_flag(sleep_overlay, LV_OBJ_FLAG_HIDDEN);
  lv_obj_set_style_bg_opa(sleep_overlay, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(sleep_overlay, 0, 0);
  lv_obj_add_event_cb(sleep_overlay, sleep_overlay_cb, LV_EVENT_CLICKED, NULL);
  /* Some versions of the tabview may not emit LV_EVENT_VALUE_CHANGED as expected
   * Attach to all events to ensure the callback is triggered and then filter inside
   * the callback if needed. */

  focus_tab(objects.tv_config, objects.tab_outras);
  if (iniciar_wifi_salvo()) {
    db_conectado();
    focus_tab(objects.tv_dashboard, objects.tab_home);
  } else {
    db_desconectado();

    focus_tab(objects.tv_dashboard, objects.tab_config, false);
    focus_tab(objects.tv_config, objects.tab_wifi);
  }

  lv_label_set_text(objects.lb_bateria, "?%");

  /* Release the mutex */
  bsp_display_unlock();
  bsp_display_backlight_on();
  bsp_display_brightness_set(25);

  #ifdef BLINK_LED
    pinMode(LED_PIN, OUTPUT);
  #endif
  ultimo_heartbeat = 0;

  timerRequisicao = lv_timer_create(timer_requisicao, tempoRequisicao * 1000, nullptr);
  lv_timer_create(battery_ui_timer_cb, 1000, nullptr);

  LOG_INFO(title, "INICIALIZADO!");
}

// L     OOO   OOO  PPPP  
// L    O   O O   O P   P 
// L    O   O O   O PPPP  
// L    O   O O   O P     
// LLLL  OOO   OOO  P     
void loop() {

  ultima_requisicao = millis();
  requisicoes_pendentes();
  escanear_redes();
<<<<<<< HEAD
  //delay(50);
=======
  if (backlight_is_on) {
    db_home_tick();
    db_rede_tick();
  }
  delay(50);
>>>>>>> 4292549 (melhorias:tema,status,alertas,tempo de tela,modal confirmacao)

  // Controle de backlight por inatividade (apaga após timeout configurado e acende com 1 toque)
  uint32_t agora = millis();
  if (backlight_is_on && (agora - last_battery_check_ms >= BATTERY_CHECK_INTERVAL_MS)) {
    last_battery_check_ms = agora;
    if (bsp_display_lock(30)) {
      atualizar_label_bateria();
      bsp_display_unlock();
    }
  }
  if (agora - last_backlight_check_ms >= BACKLIGHT_CHECK_INTERVAL_MS) {
    last_backlight_check_ms = agora;
    int timeout_s = obterTimeoutTelaSeg();
    if (bsp_display_lock(30)) {
      uint32_t inactive_ms = lv_disp_get_inactive_time(NULL);
      uint32_t timeout_ms = static_cast<uint32_t>(timeout_s) * 1000UL;
      if (timeout_s > 0 && backlight_is_on && inactive_ms >= timeout_ms) {
        backlight_is_on = false;
        bsp_display_backlight_off();
        if (sleep_overlay) {
          lv_obj_clear_flag(sleep_overlay, LV_OBJ_FLAG_HIDDEN);
          lv_obj_move_foreground(sleep_overlay);
        }
      } else if (!backlight_is_on && timeout_s <= 0) {
        backlight_is_on = true;
        bsp_display_brightness_set(obterBrilhoTela());
        if (sleep_overlay) {
          lv_obj_add_flag(sleep_overlay, LV_OBJ_FLAG_HIDDEN);
        }
      }
      bsp_display_unlock();
    }
  }
  
  //Defina como 0 para depurar
  if((millis() - ultimo_heartbeat) >= 10000) {
    ultimo_heartbeat = millis();
    if (lv_obj_has_flag(objects.pn_spinner, LV_OBJ_FLAG_HIDDEN)) {
      ocultar_spinner();
    }
    //DEBUG_TASKS();
    showHeap();

    // LOG_PING("HEARTBEAT", "Testando o LOOP principal!");
    #ifdef BLINK_LED
      digitalWrite(LED_PIN, HIGH);
      delay(50);
      digitalWrite(LED_PIN, LOW);
    #endif
  }

  static uint32_t ultimo_check_inativo = 0;
  if (millis() - ultimo_check_inativo >= 1000) {
    ultimo_check_inativo = millis();
	Serial.print("Battery voltage is ");
	Serial.print(battery.voltage());
	Serial.print(" (");
	Serial.print(battery.level());
	Serial.println("%)");

    // Pergunta ao LVGL há quantos ms o usuário não toca na tela
    uint32_t tempo_inativo = lv_disp_get_inactive_time(NULL);

    if (!tela_apagada && tempo_inativo >= (tempoScreensaver * 1000)) {
      if (bsp_display_lock(100)) {
        tela_apagada = true;
        lv_scr_load(objects.form_black);
        bsp_display_brightness_set(0); 
        // Apaga a luz de fundo completamente (0%)
        
        // Ativa o overlay transparente e joga ele para a frente de tudo
        bsp_display_unlock();
        LOG_WARN("DISPLAY", "Inatividade detectada. Tela apagada.");
      }
    }
  }
}

static void focus_tab(lv_obj_t *tabview, lv_obj_t *target_page, bool send_event) {
  uint32_t id_tab = lv_obj_get_index(target_page);
  lv_tabview_set_act(tabview, id_tab, LV_ANIM_OFF);
  if (send_event)
    lv_event_send(tabview, LV_EVENT_VALUE_CHANGED, NULL);
}

extern "C" void action_mudanca_aba(lv_event_t *e) {
  // Se o teclado estiver visível quando mudar de aba, força o fechamento seguro
  if (!lv_obj_has_flag(objects.keyboard_1, LV_OBJ_FLAG_HIDDEN)) {
    lv_event_send(objects.keyboard_1, LV_EVENT_CANCEL, NULL);
  }

  lv_obj_t *tabview = lv_event_get_target(e);
  uint16_t aba_ativa = lv_tabview_get_tab_act(tabview);

  if (tabview == objects.tv_config) {
    String tv = "tv_config";
    switch (aba_ativa) {
      case 0:
        LOG_INFO(tv, "Config selecionado");
        break;

      case 1:
        LOG_INFO(tv, "Wifi selecionado");
        // escanear_redes();
        exibir_spinner("WIFI");
        loopListaWifi = true;
        break;
    }
  } else if (tabview == objects.tv_dashboard) { 
    //! DEVE VERIFICAR INFORMAÇÕES DE ZERAMENTO NA TABVIEW CERTA!!!!!
    ultima_requisicao = 0;
    LOG_ERROR("mudanca_aba", "zerou a requisicao");
    requisicao = nullptr;

    String tv = "tv_dashboard";
    ultima_aba_aberta = aba_ativa;
    if(aba_ativa != 0) {
      //lv_timer_pause(timer1s);
    }
    switch (aba_ativa) {
      case 0: //Home
        LOG_INFO(tv, "Home selecionado");
        lv_info_home();
        break;
      case 1: //Cluster
        LOG_INFO(tv, "Cluster selecionado");
        lv_lista_clusters();
        break;
      case 2: //Docker
        LOG_INFO(tv, "Docker selecionado");
        /* code */
        requisicao = lv_lista_containers;
        break;
      case 3: //Rede
        LOG_INFO(tv, "Rede selecionado");
        requisicao = lv_info_rede;
        break;
      case 4: //Alertas
        LOG_INFO(tv, "Alertas selecionado");
        lv_info_alertas();
        break;
      case 5: //Config
        LOG_INFO(tv, "Config selecionado");
        db_config_atualizar_info();
        break;
    }
  }

  if(requisicao) {
    LOG_INFO("mudanca_aba", "tem requisicao");
    lv_timer_ready(timerRequisicao);
  }
}

extern "C" void action_atualizar_aba(lv_event_t *e) {
  lv_obj_t * tab_btns = lv_event_get_target(e);
  uint16_t aba_clicada = lv_btnmatrix_get_selected_btn(tab_btns);

  if(aba_clicada != ultima_aba_aberta) {
    // action_mudanca_aba(e);
  }
}
