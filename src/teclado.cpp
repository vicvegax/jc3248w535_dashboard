#include "teclado.h"
#include "ui/ui.h"

// Mantemos um ponteiro global para o teclado nesta aba
static lv_obj_t * teclado = NULL;

// O callback que reage aos toques no TextArea
static void evento_teclado_cb(lv_event_t * e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * ta = lv_event_get_target(e); // O alvo é o próprio TextArea

    if (code == LV_EVENT_FOCUSED || code == LV_EVENT_CLICKED) {
        
        // Se o teclado ainda não existe na memória, nós o criamos
        if (teclado == NULL) {
            // Cria o teclado na tela atual
            teclado = lv_keyboard_create(lv_scr_act()); 
            
            // (Opcional) Altera para o modo de senhas/números, ou apenas letras
            // lv_keyboard_set_mode(teclado_wifi, LV_KEYBOARD_MODE_TEXT_LOWER);
        }
        
        // Associa o teclado a este campo de texto específico
        lv_keyboard_set_textarea(teclado, ta);
        
        // Garante que ele fique visível
        lv_obj_clear_flag(teclado, LV_OBJ_FLAG_HIDDEN);
        
    } 
    else if (code == LV_EVENT_DEFOCUSED || code == LV_EVENT_READY || code == LV_EVENT_CANCEL) {
        // Se o usuário clicar fora, ou apertar o "OK" (READY) do teclado
        if (teclado != NULL) {
            lv_obj_add_flag(teclado, LV_OBJ_FLAG_HIDDEN); // Esconde o teclado
            lv_keyboard_set_textarea(teclado, NULL);      // Remove a associação
        }
    }
}

void anexar_teclado(lv_obj_t * textarea) {
    // Adiciona nosso callback ao campo de senha criado pelo EEZ Studio
    if(textarea != NULL) {
        lv_obj_add_event_cb(textarea, evento_teclado_cb, LV_EVENT_ALL, NULL);
    }
}
