#include "teclado.h"
#include "ui/ui.h"
#include <Arduino.h>

#define ALTURA_TECLADO     160 
#define TV_DASHBOARD_OPENED   284
#define TV_DASHBOARD_CLOSED   TV_DASHBOARD_OPENED - ALTURA_TECLADO
#define TV_CONFIG_OPENED   265
#define TV_CONFIG_CLOSED   TV_CONFIG_OPENED - ALTURA_TECLADO


// Função auxiliar interna para tratar o fechamento do teclado
static void fechar_teclado_event_cb(lv_event_t * e) {
  lv_event_code_t code = lv_event_get_code(e);
  
  // Verifica se o usuário clicou no botão "Ready" (✓) ou "Close" (X) do teclado
  if(code == LV_EVENT_READY || code == LV_EVENT_CANCEL || code == LV_EVENT_DEFOCUSED) {
    if (!lv_obj_has_flag(objects.keyboard_1, LV_OBJ_FLAG_HIDDEN)) {
      lv_obj_set_height(objects.tv_dashboard, TV_DASHBOARD_OPENED);
      lv_obj_set_height(objects.tv_config, TV_CONFIG_OPENED);

      // Esconde o teclado novamente
      lv_obj_add_flag(objects.keyboard_1, LV_OBJ_FLAG_HIDDEN);
    }
    // Remove o foco do textarea atual para limpar o cursor
    lv_obj_t * ta = lv_keyboard_get_textarea(objects.keyboard_1);
    if(ta) {
      lv_obj_clear_state(ta, LV_STATE_FOCUSED);
      
      // Isso garante que o próximo clique seja computado como um NOVO foco, reabrindo o teclado.
      lv_group_focus_obj(NULL); 
    }
  }
}

extern "C" void action_set_keyboard(lv_event_t *e) {
  // 1. Identifica o Textarea focado
  lv_obj_t * textarea_focado = lv_event_get_target(e); 
  
  // 2. Vincula o teclado ao Textarea
  lv_keyboard_set_textarea(objects.keyboard_1, textarea_focado); 

  uintptr_t user_data_val = (uintptr_t)lv_event_get_user_data(e);
  // Serial.println(user_data_val);
  if(user_data_val == 987654) {
    // Altera o layout do teclado para modo exclusivamente numérico (+, - e ponto)
    lv_keyboard_set_mode(objects.keyboard_1, LV_KEYBOARD_MODE_NUMBER);
  } else {
  // Retorna o layout para o teclado de texto padrão (letras minúsculas/maiúsculas)
    lv_keyboard_set_mode(objects.keyboard_1, LV_KEYBOARD_MODE_TEXT_LOWER);
  }
  //   lv_keyboard_set_textarea(objects.keyboard_1, textarea_focado); 
  
  if(lv_obj_has_flag(objects.keyboard_1, LV_OBJ_FLAG_HIDDEN)) {
    lv_obj_set_height(objects.tv_dashboard, TV_DASHBOARD_CLOSED);
    lv_obj_set_height(objects.tv_config, TV_CONFIG_CLOSED);

    // 3. Torna o teclado visível (remove a flag Hidden)
    lv_obj_clear_flag(objects.keyboard_1, LV_OBJ_FLAG_HIDDEN);
  }
  lv_obj_scroll_to_view(textarea_focado, LV_ANIM_OFF); 
  
  // 4. Adiciona um evento ao próprio teclado para saber quando o usuário terminou
  // Usamos lv_obj_remove_event_cb para evitar adicionar o mesmo evento múltiplas vezes
  lv_obj_remove_event_cb(objects.keyboard_1, fechar_teclado_event_cb);
  lv_obj_remove_event_cb(objects.keyboard_1, fechar_teclado_event_cb);
  lv_obj_add_event_cb(objects.keyboard_1, fechar_teclado_event_cb, LV_EVENT_READY, NULL);
  lv_obj_add_event_cb(objects.keyboard_1, fechar_teclado_event_cb, LV_EVENT_CANCEL, NULL);
}