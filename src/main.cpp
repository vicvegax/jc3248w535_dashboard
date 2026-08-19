
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

lv_style_t estilo_checked;
static void focus_tab(lv_obj_t *tabview, lv_obj_t *target_page, bool send_event = true);

void cb_log(const char* buf) {
  Serial.println("****GERADO PELO CALLBACK****");
  Serial.println(buf);
  Serial.println("****************************");
}

void setup() {
  // ledcAttach(1, 5000, 8);
  
   // delay(1000);
  String title = "Dashboard";

  Serial.begin(115200);
  delay(1000);
  Serial.println(title + " start");

  Serial.println("Inicializando...");
  lv_log_register_print_cb(cb_log);

  bsp_display_cfg_t cfg = {
    .lvgl_port_cfg = ESP_LVGL_PORT_INIT_CONFIG(),
    .buffer_size = EXAMPLE_LCD_QSPI_H_RES * EXAMPLE_LCD_QSPI_V_RES,
    .rotate = LV_DISP_ROT_90,
  };

  bsp_display_start_with_config(&cfg);
  // // Inicia com brilho máximo (255)
  bsp_display_backlight_on();
  bsp_display_brightness_set(25);

  Serial.println("> Criando UI");
  /* Lock the mutex due to the LVGL APIs are not thread-safe */
  bsp_display_lock(0);

  ui_init();

  lv_style_init(&estilo_checked);
  // Cor de fundo azul quando marcado
  lv_style_set_bg_color(&estilo_checked, lv_palette_main(LV_PALETTE_BLUE)); 
  lv_style_set_bg_opa(&estilo_checked, LV_OPA_COVER);
  // Texto branco quando marcado
  lv_style_set_text_color(&estilo_checked, lv_color_white()); 

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
  }

  /* Release the mutex */
  bsp_display_unlock();

  Serial.println("### " + title + " INICIALIZADO! ###");
}

int contador = 0;
void loop() {
  delay(50);
  contador++;
  if(contador >= 200) {
    contador = 0;
    Serial.print(millis());
    Serial.println(" - passou 10 segundos!");
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
    Serial.print("> tv_config - aba ");
    // Serial.println(aba_ativa);
    switch (aba_ativa) {
      case 0:
        // O usuário acabou de abrir a aba HOME
        Serial.println("HOME. Atualizando dados...");
        break;

      case 1:
        // O usuário acabou de abrir a aba CONFIG
        Serial.println("WIFI. Escaneando WiFi...");
        tab_config_escanear_redes();
        break;
    }
  } else if (tabview == objects.tv_dashboard) {
    Serial.print("> tv_dashboard - aba ");
    switch (aba_ativa) {
      case 1: //Cluster
        Serial.println("Cluster");
        lista_clusters();
        break;
      case 2: //Docker
        Serial.println("Docker");
        /* code */
        lista_containers();
        break;
      case 3: //Rede
        Serial.println("Rede");
        info_rede();
        break;
      case 4: //Alertas
        Serial.println("Alertas");
        break;
      case 5: //Config
        Serial.println("Config");
        break;
    }
  }
}

