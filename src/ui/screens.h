#ifndef EEZ_LVGL_UI_SCREENS_H
#define EEZ_LVGL_UI_SCREENS_H

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

// Screens

enum ScreensEnum {
    _SCREEN_ID_FIRST = 1,
    SCREEN_ID_MAIN = 1,
    _SCREEN_ID_LAST = 1
};

typedef struct _objects_t {
    lv_obj_t *main;
    lv_obj_t *tv_dashboard;
    lv_obj_t *tab_home;
    lv_obj_t *lb_conectado;
    lv_obj_t *tab_docker;
    lv_obj_t *ls_containers;
    lv_obj_t *tab_rede;
    lv_obj_t *tab_cluster;
    lv_obj_t *tab_alertas;
    lv_obj_t *tab_config;
    lv_obj_t *tv_config;
    lv_obj_t *tab_outras;
    lv_obj_t *ed_http;
    lv_obj_t *ed_port;
    lv_obj_t *bt_salvar_http;
    lv_obj_t *tab_wifi;
    lv_obj_t *ls_wifi;
    lv_obj_t *bt_conectar;
    lv_obj_t *ed_senha;
    lv_obj_t *bt_esquecer;
} objects_t;

extern objects_t objects;

void create_screen_main();
void tick_screen_main();

void tick_screen_by_id(enum ScreensEnum screenId);
void tick_screen(int screen_index);

void create_screens();

#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_SCREENS_H*/