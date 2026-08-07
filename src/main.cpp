
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

SemaphoreHandle_t lvgl_mutex;
lv_style_t estilo_checked;
static void focus_tab(lv_obj_t *tabview, lv_obj_t *target_page, bool send_event = true);

void setup() {
  // delay(1000);
  String title = "Dashboard";

  Serial.begin(115200);
  Serial.println(title + " start");

  Serial.println("Inicializando Display");
  bsp_display_cfg_t cfg = {
    .lvgl_port_cfg = ESP_LVGL_PORT_INIT_CONFIG(),
    .buffer_size = EXAMPLE_LCD_QSPI_H_RES * EXAMPLE_LCD_QSPI_V_RES,
    .rotate = LV_DISP_ROT_90,
  };

  bsp_display_start_with_config(&cfg);
  bsp_display_backlight_on();

  Serial.println("Criando UI");
  /* Lock the mutex due to the LVGL APIs are not thread-safe */
  bsp_display_lock(0);

  lvgl_mutex = xSemaphoreCreateMutex();
  
  ui_init();

  lv_style_init(&estilo_checked);
  // Cor de fundo azul quando marcado
  lv_style_set_bg_color(&estilo_checked, lv_palette_main(LV_PALETTE_BLUE)); 
  lv_style_set_bg_opa(&estilo_checked, LV_OPA_COVER);
  // Texto branco quando marcado
  lv_style_set_text_color(&estilo_checked, lv_color_white()); 

  // deletarCluster(0); // Limpa o cluster 0 para testes

  lv_obj_set_parent(objects.keyboard_1, lv_layer_top());
  lv_obj_add_flag(objects.keyboard_1, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(objects.pn_spinner, LV_OBJ_FLAG_HIDDEN);
  /* Some versions of the tabview may not emit LV_EVENT_VALUE_CHANGED as expected
   * Attach to all events to ensure the callback is triggered and then filter inside
   * the callback if needed. */

  focus_tab(objects.tv_config, objects.tab_outras);
  if (iniciar_wifi_salvo()) {
    db_home_conectado();
    focus_tab(objects.tv_dashboard, objects.tab_home);
  } else {
    db_home_desconectado();

    focus_tab(objects.tv_dashboard, objects.tab_config, false);
    focus_tab(objects.tv_config, objects.tab_wifi);

    // 2. Oculta ou desabilita abas
    // lv_obj_t * tab_btns = lv_tabview_get_tab_btns(objects.tv_dashboard);
    // lv_btnmatrix_set_btn_ctrl(tab_btns, 1, LV_BTNMATRIX_CTRL_DISABLED); // Desativa Docker (índice 1)
  }

  /* Release the mutex */
  bsp_display_unlock();

  Serial.println(title + " pronto!");
}

void loop() {
  //     Serial.println("IDLE loop");
  // if (xSemaphoreTake(lvgl_mutex, portMAX_DELAY) == pdTRUE) {
  //     lv_timer_handler();
  //     xSemaphoreGive(lvgl_mutex);
  // }

  delay(50);
  // requisicoes_pendentes();
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

    // lv_obj_add_flag(objects.keyboard_1, LV_OBJ_FLAG_HIDDEN);
  }

  lv_obj_t *tabview = lv_event_get_target(e);

  uint16_t aba_ativa = lv_tabview_get_tab_act(tabview);
  if (tabview == objects.tv_config) {
    Serial.print("tv_config - aba ");
    // Serial.println(aba_ativa);
    switch (aba_ativa) {
      case 0:
        // O usuário acabou de abrir a aba HOME
        Serial.println("OUTRAS aberta. Atualizando dados...");
        break;

      case 1:
        // O usuário acabou de abrir a aba CONFIG
        Serial.println("WIFI aberta. Escaneando WiFi...");
        tab_config_escanear_redes();
        break;
    }
  } else if (tabview == objects.tv_dashboard) {
    Serial.print("tv_dashboard - aba ");
    // Serial.println(aba_ativa);
    switch (aba_ativa) {
      case 1: //Cluster
        lista_clusters();
        break;
      case 2: //Docker
        /* code */
        lista_containers();
        break;
      case 3: //Rede
        info_rede();
        break;
    }
  }
}

