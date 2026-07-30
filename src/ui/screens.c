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
            // lb_conectado
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.lb_conectado = obj;
            lv_obj_set_pos(obj, 10, 9);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_label_set_text_static(obj, "Mostra se esta conectado");
        }
        {
            // tv_dashboard
            lv_obj_t *obj = lv_tabview_create(parent_obj, LV_DIR_LEFT, 90);
            objects.tv_dashboard = obj;
            lv_obj_set_pos(obj, 0, 36);
            lv_obj_set_size(obj, 480, 284);
            lv_obj_add_event_cb(obj, action_mudanca_aba, LV_EVENT_VALUE_CHANGED, (void *)0);
            lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICKABLE);
            {
                lv_obj_t *parent_obj = obj;
                {
                    // tab_home
                    lv_obj_t *obj = lv_tabview_add_tab(parent_obj, "Home");
                    objects.tab_home = obj;
                }
                {
                    // tab_cluster
                    lv_obj_t *obj = lv_tabview_add_tab(parent_obj, "Cluster");
                    objects.tab_cluster = obj;
                    lv_obj_add_state(obj, LV_STATE_DISABLED);
                    {
                        lv_obj_t *parent_obj = obj;
                        {
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            lv_obj_set_pos(obj, 0, -8);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_label_set_text_static(obj, "Nos:");
                        }
                        {
                            // ls_nos
                            lv_obj_t *obj = lv_list_create(parent_obj);
                            objects.ls_nos = obj;
                            lv_obj_set_pos(obj, 0, 8);
                            lv_obj_set_size(obj, 180, 100);
                        }
                        {
                            // bt_no_add
                            lv_obj_t *obj = lv_btn_create(parent_obj);
                            objects.bt_no_add = obj;
                            lv_obj_set_pos(obj, 0, 194);
                            lv_obj_set_size(obj, 100, 50);
                            {
                                lv_obj_t *parent_obj = obj;
                                {
                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                    lv_obj_set_pos(obj, 0, 0);
                                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                    lv_label_set_text_static(obj, "Adicionar");
                                }
                            }
                        }
                        {
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            lv_obj_set_pos(obj, 127, 128);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_label_set_text_static(obj, "HTTP");
                        }
                        {
                            // ed_no_http
                            lv_obj_t *obj = lv_textarea_create(parent_obj);
                            objects.ed_no_http = obj;
                            lv_obj_set_pos(obj, 127, 144);
                            lv_obj_set_size(obj, 150, 40);
                            lv_textarea_set_max_length(obj, 128);
                            lv_textarea_set_one_line(obj, true);
                            lv_textarea_set_password_mode(obj, false);
                            lv_obj_add_event_cb(obj, action_set_keyboard, LV_EVENT_FOCUSED, (void *)0);
                        }
                        {
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            lv_obj_set_pos(obj, 278, 126);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_label_set_text_static(obj, "Port");
                        }
                        {
                            // ed_no_port
                            lv_obj_t *obj = lv_textarea_create(parent_obj);
                            objects.ed_no_port = obj;
                            lv_obj_set_pos(obj, 278, 144);
                            lv_obj_set_size(obj, 80, 40);
                            lv_textarea_set_accepted_chars(obj, "0123456789");
                            lv_textarea_set_max_length(obj, 5);
                            lv_textarea_set_one_line(obj, true);
                            lv_textarea_set_password_mode(obj, false);
                            lv_obj_add_event_cb(obj, action_set_keyboard, LV_EVENT_FOCUSED, (void *)987654);
                        }
                        {
                            // lb_no_temp
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.lb_no_temp = obj;
                            lv_obj_set_pos(obj, 190, 8);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_label_set_text_static(obj, "Temp.");
                        }
                        {
                            // lb_no_cpu
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.lb_no_cpu = obj;
                            lv_obj_set_pos(obj, 191, 42);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_label_set_text_static(obj, "CPU");
                        }
                        {
                            // lb_no_ram
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.lb_no_ram = obj;
                            lv_obj_set_pos(obj, 191, 72);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_label_set_text_static(obj, "RAM");
                        }
                        {
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            lv_obj_set_pos(obj, 0, 128);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_label_set_text_static(obj, "Nome");
                        }
                        {
                            // ed_no_nome
                            lv_obj_t *obj = lv_textarea_create(parent_obj);
                            objects.ed_no_nome = obj;
                            lv_obj_set_pos(obj, 0, 144);
                            lv_obj_set_size(obj, 126, 40);
                            lv_textarea_set_max_length(obj, 128);
                            lv_textarea_set_one_line(obj, true);
                            lv_textarea_set_password_mode(obj, false);
                            lv_obj_add_event_cb(obj, action_set_keyboard, LV_EVENT_FOCUSED, (void *)0);
                        }
                        {
                            // bt_salvar_http
                            lv_obj_t *obj = lv_btn_create(parent_obj);
                            objects.bt_salvar_http = obj;
                            lv_obj_set_pos(obj, 179, 194);
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
                    // tab_docker
                    lv_obj_t *obj = lv_tabview_add_tab(parent_obj, "Docker");
                    objects.tab_docker = obj;
                    lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICKABLE);
                    {
                        lv_obj_t *parent_obj = obj;
                        {
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            lv_obj_set_pos(obj, 0, -7);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_label_set_text_static(obj, "Ativos");
                        }
                        {
                            // ls_cont_ativos
                            lv_obj_t *obj = lv_list_create(parent_obj);
                            objects.ls_cont_ativos = obj;
                            lv_obj_set_pos(obj, 0, 9);
                            lv_obj_set_size(obj, 180, 96);
                        }
                        {
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            lv_obj_set_pos(obj, 0, 118);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_label_set_text_static(obj, "Inativos");
                        }
                        {
                            // ls_cont_inativos
                            lv_obj_t *obj = lv_list_create(parent_obj);
                            objects.ls_cont_inativos = obj;
                            lv_obj_set_pos(obj, 0, 134);
                            lv_obj_set_size(obj, 180, 96);
                        }
                        {
                            // ed_cont_cpu
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.ed_cont_cpu = obj;
                            lv_obj_set_pos(obj, 198, 9);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_label_set_text_static(obj, "CPU");
                        }
                        {
                            // ed_cont_ram
                            lv_obj_t *obj = lv_label_create(parent_obj);
                            objects.ed_cont_ram = obj;
                            lv_obj_set_pos(obj, 198, 49);
                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                            lv_label_set_text_static(obj, "RAM");
                        }
                    }
                }
                {
                    // tab_rede
                    lv_obj_t *obj = lv_tabview_add_tab(parent_obj, "Rede");
                    objects.tab_rede = obj;
                }
                {
                    // tab_alertas
                    lv_obj_t *obj = lv_tabview_add_tab(parent_obj, "Alertas");
                    objects.tab_alertas = obj;
                }
                {
                    // tab_config
                    lv_obj_t *obj = lv_tabview_add_tab(parent_obj, "Config");
                    objects.tab_config = obj;
                    {
                        lv_obj_t *parent_obj = obj;
                        {
                            // tv_config
                            lv_obj_t *obj = lv_tabview_create(parent_obj, LV_DIR_TOP, 32);
                            objects.tv_config = obj;
                            lv_obj_set_pos(obj, -15, -16);
                            lv_obj_set_size(obj, 373, 265);
                            lv_obj_add_event_cb(obj, action_mudanca_aba, LV_EVENT_VALUE_CHANGED, (void *)0);
                            {
                                lv_obj_t *parent_obj = obj;
                                {
                                    // tab_outras
                                    lv_obj_t *obj = lv_tabview_add_tab(parent_obj, "Outras");
                                    objects.tab_outras = obj;
                                    {
                                        lv_obj_t *parent_obj = obj;
                                        {
                                            lv_obj_t *obj = lv_btn_create(parent_obj);
                                            lv_obj_set_pos(obj, 97, 9);
                                            lv_obj_set_size(obj, 140, 50);
                                            lv_obj_add_event_cb(obj, action_resetar, LV_EVENT_PRESSED, (void *)0);
                                            {
                                                lv_obj_t *parent_obj = obj;
                                                {
                                                    lv_obj_t *obj = lv_label_create(parent_obj);
                                                    lv_obj_set_pos(obj, 0, 0);
                                                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                                                    lv_label_set_text_static(obj, "Resetar ESP32");
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
                                            lv_obj_set_pos(obj, 0, 10);
                                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                            lv_label_set_text_static(obj, "Rede WIFI:");
                                        }
                                        {
                                            // ls_wifi
                                            lv_obj_t *obj = lv_dropdown_create(parent_obj);
                                            objects.ls_wifi = obj;
                                            lv_obj_set_pos(obj, 103, -5);
                                            lv_obj_set_size(obj, 150, LV_SIZE_CONTENT);
                                            lv_dropdown_set_options_static(obj, "INDEFINIDO");
                                            lv_dropdown_set_selected(obj, 0);
                                        }
                                        {
                                            lv_obj_t *obj = lv_label_create(parent_obj);
                                            lv_obj_set_pos(obj, 0, 58);
                                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                            lv_label_set_text_static(obj, "Senha:");
                                        }
                                        {
                                            // ed_wifi_senha
                                            lv_obj_t *obj = lv_textarea_create(parent_obj);
                                            objects.ed_wifi_senha = obj;
                                            lv_obj_set_pos(obj, 103, 43);
                                            lv_obj_set_size(obj, 150, 40);
                                            lv_textarea_set_max_length(obj, 128);
                                            lv_textarea_set_one_line(obj, true);
                                            lv_textarea_set_password_mode(obj, true);
                                            lv_obj_add_event_cb(obj, action_set_keyboard, LV_EVENT_FOCUSED, (void *)0);
                                        }
                                        {
                                            // bt_conectar
                                            lv_obj_t *obj = lv_btn_create(parent_obj);
                                            objects.bt_conectar = obj;
                                            lv_obj_set_pos(obj, 145, 110);
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
                                            // bt_esquecer
                                            lv_obj_t *obj = lv_btn_create(parent_obj);
                                            objects.bt_esquecer = obj;
                                            lv_obj_set_pos(obj, -5, 110);
                                            lv_obj_set_size(obj, 131, 52);
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
                                        {
                                            // ch_ver_senha
                                            lv_obj_t *obj = lv_checkbox_create(parent_obj);
                                            objects.ch_ver_senha = obj;
                                            lv_obj_set_pos(obj, 264, 61);
                                            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                                            lv_checkbox_set_text_static(obj, "VER");
                                            lv_obj_add_event_cb(obj, action_mostrar_senha, LV_EVENT_VALUE_CHANGED, (void *)0);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        {
            // pn_spinner
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.pn_spinner = obj;
            lv_obj_set_pos(obj, 0, 0);
            lv_obj_set_size(obj, 480, 320);
            lv_obj_set_style_bg_opa(obj, 200, LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    // spinner_1
                    lv_obj_t *obj = lv_spinner_create(parent_obj, 1000, 60);
                    objects.spinner_1 = obj;
                    lv_obj_set_pos(obj, 182, 102);
                    lv_obj_set_size(obj, 80, 80);
                    lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICK_FOCUSABLE);
                }
            }
        }
        {
            // keyboard_1
            lv_obj_t *obj = lv_keyboard_create(parent_obj);
            objects.keyboard_1 = obj;
            lv_obj_set_pos(obj, 0, 160);
            lv_obj_set_size(obj, 480, 160);
            lv_obj_set_style_align(obj, LV_ALIGN_DEFAULT, LV_PART_MAIN | LV_STATE_DEFAULT);
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