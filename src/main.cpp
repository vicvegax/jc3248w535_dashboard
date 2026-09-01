
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
#include "logger.h"

#define DEBUG_TASKS() \
    Serial.printf("Stack livre Loop: %d | Stack livre GUI: %d\n", \
    uxTaskGetStackHighWaterMark(NULL), \
    uxTaskGetStackHighWaterMark(xTaskGetHandle("LVGL task"))) // ou o nome que o bsp usa


#define BLINK_LED
#define LED_PIN         (17)
#define BATT_PIN         (5)
#define REFERENCIA_V     (3.3)       // Tensão de referência do ADC
#define RESOLUCAO_ADC (4095.0)       // 12-bits de resolução (0 a 4095)
#define FATOR_DIVISOR    (1.77)      // Divisor de tensão físico na placa (proporção 2:1)
#define TENSAO_MAX       (4.15)
#define TENSAO_MIN       (3.15)
#define ALFA             (0.03)

char bateria[8];

uint32_t ultima_bateria;
uint32_t ultimo_heartbeat;
static float ler_bateria_bruta();
static int ler_bateria_suave();

lv_style_t estilo_checked;
static void focus_tab(lv_obj_t *tabview, lv_obj_t *target_page, bool send_event = true);
static uint16_t ultima_aba_aberta = -1;

void cb_log(const char* buf) {
  Serial.println("****GERADO PELO CALLBACK****");
  Serial.println(buf);
  Serial.println("****************************");
}

void setup() {
  String title = "Dashboard";

  Serial.begin(115200);
  LOG_INFO(title, "INICIALIZANDO...");

  lv_log_register_print_cb(cb_log);

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
  // // Inicia com brilho máximo (255)

  LOG_INFO(title, "Criando UI...");
  /* Lock the mutex due to the LVGL APIs are not thread-safe */
  bsp_display_lock(0); //SETUP

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
  ultima_bateria = millis() - 20000;
  ultimo_heartbeat = 0;
  LOG_INFO(title, "INICIALIZADO!");
}

void loop() {
  requisicoes_pendentes();
  escanear_redes();
  delay(50);

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

  ler_bateria_bruta();
  if((millis() - ultima_bateria) >= 30000) {
    ultima_bateria = millis();
    if (!lv_obj_has_flag(objects.keyboard_1, LV_OBJ_FLAG_HIDDEN)) {
      LOG_WARN("BATERIA", "Teclado aberto; pulando atualização da label por 10 segundos.");
      ultima_bateria -= 20000; //hack para aguardar 10 segundos e tentar de novo
      return;
    }

    int bateria_pct = ler_bateria_suave();

    // LOG_INFO("BATT", "Nivel atual: %d%%", bateria_pct);

    snprintf(bateria, sizeof(bateria), "%d%%", bateria_pct);

    if(bsp_display_lock(150)) {
      lv_label_set_text(objects.lb_bateria, bateria);
      bsp_display_unlock();
    } else {
      ultima_bateria -= 20000; //hack para aguardar 10 segundos e tentar de novo
      LOG_ERROR("BATERIA", "LVGL muito ocupado; pulando atualização por 10 segundos.");
    }
  }
  
}

static float ler_bateria_bruta() {
  // static mantém o valor da leitura anterior guardado na memória
  static float valorFiltrado = -1.0; 
  
  // Lê o ADC apenas UMA vez (sem travar com laço for ou delays)
  int leituraBruta = analogRead(BATT_PIN); 
  
  // Inicialização na primeira corrida
  if (valorFiltrado < 0.0) {
    long soma = leituraBruta;
    
    // Tira 5 amostras rápidas consecutivas para estabilizar o pino
    for (int i = 0; i < 5; i++) {
      soma += analogRead(BATT_PIN);
      delay(2); // Meio milissegundo entre leituras
    }
    
    // Média inicial sólida
    valorFiltrado = (float)(soma / 6.0f);
    
    return valorFiltrado;
  }


  // Fator de suavização (Alfa). Quanto MENOR, mais estável e lento o filtro fica.
  // 0.05 significa que a nova leitura tem peso de 5% e o histórico tem peso de 95%
  // Aplica a fórmula do filtro EMA
  valorFiltrado = (ALFA * (float)leituraBruta) + ((1.0 - ALFA) * valorFiltrado);

  return valorFiltrado;
}

static int ler_bateria_suave() {
  float valorBruto = ler_bateria_bruta();
  float tensaoPino = (valorBruto * REFERENCIA_V / RESOLUCAO_ADC);
  float tensaoBat = tensaoPino * FATOR_DIVISOR;

  LOG_INFO("BATERIA", "RAW: %.0f, VPin: %.2f, VBat: %.2f", valorBruto, tensaoPino, tensaoBat);

  if (tensaoBat >= TENSAO_MAX) return 100;
  if (tensaoBat <= TENSAO_MIN) return 0;

  // Mapeamento linear básico (para precisão total seria necessária uma tabela lookup)
  return (int)((tensaoBat - TENSAO_MIN) / (TENSAO_MAX - TENSAO_MIN) * 100.0);
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
    //ultima_aba_aberta = -1;
    String tv = "tv_config";
    switch (aba_ativa) {
      case 0:
        LOG_INFO(tv, "Config selecionado");
        break;

      case 1:
        LOG_INFO(tv, "Wifi selecionado");
        // escanear_redes();
        exibir_spinner();
        loopListaWifi = true;
        break;
    }
  } else if (tabview == objects.tv_dashboard) {
    String tv = "tv_dashboard";
    ultima_aba_aberta = aba_ativa;
    switch (aba_ativa) {
      case 0: //Home
        LOG_INFO(tv, "Home selecionado");
        break;
      case 1: //Cluster
        LOG_INFO(tv, "Cluster selecionado");
        lv_lista_clusters();
        break;
      case 2: //Docker
        LOG_INFO(tv, "Docker selecionado");
        /* code */
        lv_lista_containers();
        break;
      case 3: //Rede
        LOG_INFO(tv, "Rede selecionado");
        lv_info_rede();
        break;
      case 4: //Alertas
        LOG_INFO(tv, "Alertas selecionado");
        break;
      case 5: //Config
        LOG_INFO(tv, "Config selecionado");
        break;
    }
  }
}

extern "C" void action_atualizar_aba(lv_event_t *e) {
  lv_obj_t * tab_btns = lv_event_get_target(e);
  uint16_t aba_clicada = lv_btnmatrix_get_selected_btn(tab_btns);

  if(aba_clicada != ultima_aba_aberta) {
    // action_mudanca_aba(e);
  }
}
