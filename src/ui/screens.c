#include <string.h>

#include "screens.h"
#include "images.h"
#include "fonts.h"
#include "actions.h"
#include "vars.h"
#include "styles.h"
#include "ui.h"

#include <string.h>

objects_t objects;

//
// Event handlers
//

lv_obj_t *tick_value_change_obj;

//
// Screens
//

void create_screen_main() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.main = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 480, 320);
    {
        lv_obj_t *parent_obj = obj;
        {
            // tv_dashboard
            lv_obj_t *obj = lv_tabview_create(parent_obj, LV_DIR_LEFT, 90);
            objects.tv_dashboard = obj;
            lv_obj_set_pos(obj, 0, 0);
            lv_obj_set_size(obj, 480, 320);
            lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICKABLE);
            {
                lv_obj_t *parent_obj = obj;
                {
                    // tabHome
                    lv_obj_t *obj = lv_tabview_add_tab(parent_obj, "Home");
                    objects.tab_home = obj;
                    {
                        lv_obj_t *parent_obj = obj;
                        {
                            // lb_conectado
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.lb_conectado = obj;
                            lv_obj_set_pos(obj, 32, 40);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_label_set_text_static(obj, "Mostra se esta conectado");
                        }
                    }
                }
                {
                    // tabDocker
                    lv_obj_t *obj = lv_tabview_add_tab(parent_obj, "Docker");
                    objects.tab_docker = obj;
                    lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICKABLE);
                    {
                        lv_obj_t *parent_obj = obj;
                        {
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            lv_obj_set_pos(obj, 2, 43);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_label_set_text_static(obj, "Lista");
                        }
                        {
                            // ls_containers
                            lv_obj_t *obj = lv_list_create(parent_obj);
                            objects.ls_containers = obj;
                            lv_obj_set_pos(obj, 0, 64);
                            lv_obj_set_size(obj, 180, 184);
                        }
                    }
                }
                {
                    // tabRede
                    lv_obj_t *obj = lv_tabview_add_tab(parent_obj, "Rede");
                    objects.tab_rede = obj;
                    {
                        lv_obj_t *parent_obj = obj;
                        {
                            lv_obj_t *obj = lv_btn_create(parent_obj);
                            lv_obj_set_pos(obj, 129, 103);
                            lv_obj_set_size(obj, 100, 50);
                            lv_obj_add_event_cb(obj, action_resetar, LV_EVENT_PRESSED, (void *)0);
                            {
                                lv_obj_t *parent_obj = obj;
                                {
                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                    lv_obj_set_pos(obj, 0, 0);
                                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_label_set_text_static(obj, "Resetar");
                                }
                            }
                        }
                    }
                }
                {
                    // tabCluster
                    lv_obj_t *obj = lv_tabview_add_tab(parent_obj, "Cluster");
                    objects.tab_cluster = obj;
                    lv_obj_add_state(obj, LV_STATE_DISABLED);
                }
                {
                    // tabAlertas
                    lv_obj_t *obj = lv_tabview_add_tab(parent_obj, "Alertas");
                    objects.tab_alertas = obj;
                }
                {
                    // tab_config
                    lv_obj_t *obj = lv_tabview_add_tab(parent_obj, "Config");
                    objects.tab_config = obj;
                    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
                    {
                        lv_obj_t *parent_obj = obj;
                        {
                            // tv_config
                            lv_obj_t *obj = lv_tabview_create(parent_obj, LV_DIR_TOP, 32);
                            objects.tv_config = obj;
                            lv_obj_set_pos(obj, -15, -16);
                            lv_obj_set_size(obj, 389, 320);
                            {
                                lv_obj_t *parent_obj = obj;
                                {
                                    // tab_outras
                                    lv_obj_t *obj = lv_tabview_add_tab(parent_obj, "Outras");
                                    objects.tab_outras = obj;
                                    {
                                        lv_obj_t *parent_obj = obj;
                                        {
                                            lv_obj_t *obj = lv_label_create(parent_obj);
                                            lv_obj_set_pos(obj, -1, 24);
                                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                            lv_label_set_text_static(obj, "HTTP");
                                        }
                                        {
                                            // ed_http
                                            lv_obj_t *obj = lv_textarea_create(parent_obj);
                                            objects.ed_http = obj;
                                            lv_obj_set_pos(obj, 107, 11);
                                            lv_obj_set_size(obj, 150, 40);
                                            lv_textarea_set_max_length(obj, 128);
                                            lv_textarea_set_one_line(obj, true);
                                            lv_textarea_set_password_mode(obj, false);
                                        }
                                        {
                                            lv_obj_t *obj = lv_label_create(parent_obj);
                                            lv_obj_set_pos(obj, -1, 73);
                                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                            lv_label_set_text_static(obj, "Port");
                                        }
                                        {
                                            // ed_port
                                            lv_obj_t *obj = lv_textarea_create(parent_obj);
                                            objects.ed_port = obj;
                                            lv_obj_set_pos(obj, 108, 61);
                                            lv_obj_set_size(obj, 150, 40);
                                            lv_textarea_set_accepted_chars(obj, "0123456789");
                                            lv_textarea_set_max_length(obj, 5);
                                            lv_textarea_set_one_line(obj, true);
                                            lv_textarea_set_password_mode(obj, false);
                                        }
                                        {
                                            // bt_salvar_http
                                            lv_obj_t *obj = lv_btn_create(parent_obj);
                                            objects.bt_salvar_http = obj;
                                            lv_obj_set_pos(obj, 79, 149);
                                            lv_obj_set_size(obj, 176, 50);
                                            lv_obj_add_event_cb(obj, action_salvar_config, LV_EVENT_PRESSED, (void *)0);
                                            {
                                                lv_obj_t *parent_obj = obj;
                                                {
                                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                                    lv_obj_set_pos(obj, 0, 0);
                                                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_label_set_text_static(obj, "Salvar");
                                                }
                                            }
                                        }
                                    }
                                }
                                {
                                    // tab_wifi
                                    lv_obj_t *obj = lv_tabview_add_tab(parent_obj, "Wifi");
                                    objects.tab_wifi = obj;
                                    {
                                        lv_obj_t *parent_obj = obj;
                                        {
                                            lv_obj_t *obj = lv_label_create(parent_obj);
                                            lv_obj_set_pos(obj, -3, 81);
                                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                            lv_label_set_text_static(obj, "Senha:");
                                        }
                                        {
                                            // ls_wifi
                                            lv_obj_t *obj = lv_dropdown_create(parent_obj);
                                            objects.ls_wifi = obj;
                                            lv_obj_set_pos(obj, 145, 0);
                                            lv_obj_set_size(obj, 150, LV_SIZE_CONTENT);
                                            lv_dropdown_set_options_static(obj, "INDEFINIDO");
                                            lv_dropdown_set_selected(obj, 0);
                                        }
                                        {
                                            // bt_conectar
                                            lv_obj_t *obj = lv_btn_create(parent_obj);
                                            objects.bt_conectar = obj;
                                            lv_obj_set_pos(obj, 85, 195);
                                            lv_obj_set_size(obj, 186, 50);
                                            lv_obj_add_event_cb(obj, action_conectar_wifi, LV_EVENT_PRESSED, (void *)0);
                                            {
                                                lv_obj_t *parent_obj = obj;
                                                {
                                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                                    lv_obj_set_pos(obj, 0, 0);
                                                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_label_set_text_static(obj, "Conectar");
                                                }
                                            }
                                        }
                                        {
                                            lv_obj_t *obj = lv_label_create(parent_obj);
                                            lv_obj_set_pos(obj, -3, 24);
                                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                            lv_label_set_text_static(obj, "Rede WIFI:");
                                        }
                                        {
                                            // ed_senha
                                            lv_obj_t *obj = lv_textarea_create(parent_obj);
                                            objects.ed_senha = obj;
                                            lv_obj_set_pos(obj, 145, 61);
                                            lv_obj_set_size(obj, 150, 40);
                                            lv_textarea_set_max_length(obj, 128);
                                            lv_textarea_set_one_line(obj, true);
                                            lv_textarea_set_password_mode(obj, true);
                                        }
                                        {
                                            // bt_esquecer
                                            lv_obj_t *obj = lv_btn_create(parent_obj);
                                            objects.bt_esquecer = obj;
                                            lv_obj_set_pos(obj, 98, 128);
                                            lv_obj_set_size(obj, 161, 52);
                                            lv_obj_add_event_cb(obj, action_esquecer_wifi, LV_EVENT_PRESSED, (void *)0);
                                            {
                                                lv_obj_t *parent_obj = obj;
                                                {
                                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                                    lv_obj_set_pos(obj, 0, 0);
                                                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_label_set_text_static(obj, "Esquecer Wifi");
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    
    tick_screen_main();
}

void tick_screen_main() {
}

typedef void (*tick_screen_func_t)();
tick_screen_func_t tick_screen_funcs[] = {
    tick_screen_main,
};
void tick_screen(int screen_index) {
    if (screen_index >= 0 && screen_index < 1) {
        tick_screen_funcs[screen_index]();
    }
}
void tick_screen_by_id(enum ScreensEnum screenId) {
    tick_screen(screenId - 1);
}

//
// Fonts
//

ext_font_desc_t fonts[] = {
#if LV_FONT_MONTSERRAT_8
    { "MONTSERRAT_8", &lv_font_montserrat_8 },
#endif
#if LV_FONT_MONTSERRAT_10
    { "MONTSERRAT_10", &lv_font_montserrat_10 },
#endif
#if LV_FONT_MONTSERRAT_12
    { "MONTSERRAT_12", &lv_font_montserrat_12 },
#endif
#if LV_FONT_MONTSERRAT_14
    { "MONTSERRAT_14", &lv_font_montserrat_14 },
#endif
#if LV_FONT_MONTSERRAT_16
    { "MONTSERRAT_16", &lv_font_montserrat_16 },
#endif
#if LV_FONT_MONTSERRAT_18
    { "MONTSERRAT_18", &lv_font_montserrat_18 },
#endif
#if LV_FONT_MONTSERRAT_20
    { "MONTSERRAT_20", &lv_font_montserrat_20 },
#endif
#if LV_FONT_MONTSERRAT_22
    { "MONTSERRAT_22", &lv_font_montserrat_22 },
#endif
#if LV_FONT_MONTSERRAT_24
    { "MONTSERRAT_24", &lv_font_montserrat_24 },
#endif
#if LV_FONT_MONTSERRAT_26
    { "MONTSERRAT_26", &lv_font_montserrat_26 },
#endif
#if LV_FONT_MONTSERRAT_28
    { "MONTSERRAT_28", &lv_font_montserrat_28 },
#endif
#if LV_FONT_MONTSERRAT_30
    { "MONTSERRAT_30", &lv_font_montserrat_30 },
#endif
#if LV_FONT_MONTSERRAT_32
    { "MONTSERRAT_32", &lv_font_montserrat_32 },
#endif
#if LV_FONT_MONTSERRAT_34
    { "MONTSERRAT_34", &lv_font_montserrat_34 },
#endif
#if LV_FONT_MONTSERRAT_36
    { "MONTSERRAT_36", &lv_font_montserrat_36 },
#endif
#if LV_FONT_MONTSERRAT_38
    { "MONTSERRAT_38", &lv_font_montserrat_38 },
#endif
#if LV_FONT_MONTSERRAT_40
    { "MONTSERRAT_40", &lv_font_montserrat_40 },
#endif
#if LV_FONT_MONTSERRAT_42
    { "MONTSERRAT_42", &lv_font_montserrat_42 },
#endif
#if LV_FONT_MONTSERRAT_44
    { "MONTSERRAT_44", &lv_font_montserrat_44 },
#endif
#if LV_FONT_MONTSERRAT_46
    { "MONTSERRAT_46", &lv_font_montserrat_46 },
#endif
#if LV_FONT_MONTSERRAT_48
    { "MONTSERRAT_48", &lv_font_montserrat_48 },
#endif
};

//
// Color themes
//

uint32_t active_theme_index = 0;

//
//
//

void create_screens() {

// Set default LVGL theme
    lv_disp_t *dispp = lv_disp_get_default();
    lv_theme_t *theme = lv_theme_default_init(dispp, lv_palette_main(LV_PALETTE_BLUE), lv_palette_main(LV_PALETTE_RED), false, LV_FONT_DEFAULT);
    lv_disp_set_theme(dispp, theme);
    
    // Initialize screens
    // Create screens
    create_screen_main();
}