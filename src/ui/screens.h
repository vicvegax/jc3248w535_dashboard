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
    lv_obj_t *lb_conectado;
    lv_obj_t *tv_dashboard;
    lv_obj_t *tab_home;
    lv_obj_t *tab_cluster;
    lv_obj_t *ls_nos;
    lv_obj_t *bt_no_add;
    lv_obj_t *ed_no_http;
    lv_obj_t *ed_no_port;
    lv_obj_t *lb_no_temp;
    lv_obj_t *lb_no_cpu;
    lv_obj_t *lb_no_ram;
    lv_obj_t *ed_no_nome;
    lv_obj_t *bt_salvar_http;
    lv_obj_t *tab_docker;
    lv_obj_t *ls_cont_ativos;
    lv_obj_t *ls_cont_inativos;
    lv_obj_t *ed_cont_cpu;
    lv_obj_t *ed_cont_ram;
    lv_obj_t *tab_rede;
    lv_obj_t *tab_alertas;
    lv_obj_t *tab_config;
    lv_obj_t *tv_config;
    lv_obj_t *tab_outras;
    lv_obj_t *tab_wifi;
    lv_obj_t *ls_wifi;
    lv_obj_t *ed_wifi_senha;
    lv_obj_t *bt_conectar;
    lv_obj_t *bt_esquecer;
    lv_obj_t *ch_ver_senha;
    lv_obj_t *pn_spinner;
    lv_obj_t *spinner_1;
    lv_obj_t *keyboard_1;
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