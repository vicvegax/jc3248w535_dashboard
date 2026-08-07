#include <WiFi.h>
#include "ui/ui.h"
#include "ui/actions.h" // Para as ações dos botões
#include "db_config.h"
#include "config.h"
#include "db_home.h"
#include <Esp.h>

extern "C" void action_resetar(lv_event_t *e) {
    // TODO: Implement action resetar here
    ESP.restart();
}
// Preenche o Dropdown com redes disponíveis
void tab_config_escanear_redes() {
  exibir_spinner();  
  
  int n = WiFi.scanNetworks();
  String opcoes = "";
  for (int i = 0; i < n; ++i) {
      opcoes += WiFi.SSID(i) + "\n";
  }
  
  lv_dropdown_set_options(objects.ls_wifi, opcoes.c_str());
  ocultar_spinner();
}

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
  lv_label_set_text(label, "Conectando...");
  //DELAY PARA ATUALIZAR O LABEL ANTES DE CONECTAR
  lv_refr_now(NULL);
  // Pega o texto selecionado no Dropdown
  char ssid_selecionado[64];
  lv_dropdown_get_selected_str(objects.ls_wifi, ssid_selecionado, sizeof(ssid_selecionado));
  
  // Pega a senha do Text Area
  // OBS: Troque 'ui_TextAreaSenha' pelo nome do seu campo de texto
  const char* senha_digitada = lv_textarea_get_text(objects.ed_wifi_senha);
  // Chama a função do wifi_manager
  if(salvar_e_conectar(String(ssid_selecionado), String(senha_digitada))) {
    db_home_conectado();
    lv_label_set_text(label, "Conectar");

    lv_async_call(ir_para_home_async, NULL);
  } else {
    db_home_desconectado();
    // Mostre uma mensagem de erro (MsgBox ou Label)
    lv_label_set_text(label, "Conectar...");

    Serial.println("Falha ao conectar! Verifique SSID e senha.");
  }

}

extern "C" void action_esquecer_wifi(lv_event_t * e) {
  esquecer_wifi();
  db_home_desconectado();
  
  lv_obj_t * tab_btns = lv_tabview_get_tab_btns(objects.tv_dashboard);
  lv_btnmatrix_set_btn_ctrl(tab_btns, 1, LV_BTNMATRIX_CTRL_DISABLED); // Desativa Alertas (índice 1)
  lv_obj_t * label = lv_obj_get_child(objects.bt_esquecer, 0);
  lv_label_set_text(label, "Esquecido...");
}

extern "C" void action_mostrar_senha(lv_event_t *e) {
  // TODO: Implement action mostrar_senha here}
  lv_obj_t * cb = lv_event_get_target(e);
  lv_textarea_set_password_mode(objects.ed_wifi_senha, !lv_obj_has_state(cb, LV_STATE_CHECKED));
}