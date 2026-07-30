#ifndef EEZ_LVGL_UI_EVENTS_H
#define EEZ_LVGL_UI_EVENTS_H

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

extern void action_conectar_wifi(lv_event_t * e);
extern void action_esquecer_wifi(lv_event_t * e);
extern void action_resetar(lv_event_t * e);
extern void action_salvar_config(lv_event_t * e);
extern void action_mostrar_senha(lv_event_t * e);
extern void action_set_keyboard(lv_event_t * e);
extern void action_mudanca_aba(lv_event_t * e);

#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_EVENTS_H*/