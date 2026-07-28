
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

/**
 * Set the rotation degree:
 *      - 0: 0 degree
 *      - 90: 90 degree
 *      - 180: 180 degree
 *      - 270: 270 degree
 *
 */
#define LVGL_PORT_ROTATION_DEGREE               (90)
static void evento_mudanca_aba_cb(lv_event_t * e);
static void focus_tab(lv_obj_t * tabview, lv_obj_t * target_page, bool send_event = true);
static uint16_t get_tab_id(lv_obj_t * tabview, lv_obj_t * target_page);

void setup()
{
    delay(3000); 
    String title = "Dashboard";

    Serial.begin(115200);
    Serial.println(title + " start");

    Serial.println("Inicializando Display");
    bsp_display_cfg_t cfg = {
        .lvgl_port_cfg = ESP_LVGL_PORT_INIT_CONFIG(),
        .buffer_size = EXAMPLE_LCD_QSPI_H_RES * EXAMPLE_LCD_QSPI_V_RES,
#if LVGL_PORT_ROTATION_DEGREE == 90
        .rotate = LV_DISP_ROT_90,
#elif LVGL_PORT_ROTATION_DEGREE == 270
        .rotate = LV_DISP_ROT_270,
#elif LVGL_PORT_ROTATION_DEGREE == 180
        .rotate = LV_DISP_ROT_180,
#elif LVGL_PORT_ROTATION_DEGREE == 0
        .rotate = LV_DISP_ROT_NONE,
#endif
    };

    bsp_display_start_with_config(&cfg);
    bsp_display_backlight_on();

    Serial.println("Criando UI");
    /* Lock the mutex due to the LVGL APIs are not thread-safe */
    bsp_display_lock(0);

    ui_init();
    anexar_teclado(objects.ed_senha);
    anexar_teclado(objects.ed_http);
    anexar_teclado(objects.ed_port);
    
    /* Some versions of the tabview may not emit LV_EVENT_VALUE_CHANGED as expected
     * Attach to all events to ensure the callback is triggered and then filter inside
     * the callback if needed. */
    lv_obj_add_event_cb(objects.tv_config, evento_mudanca_aba_cb, LV_EVENT_VALUE_CHANGED, NULL);

    focus_tab(objects.tv_config, objects.tab_outras);
    if(iniciar_wifi_salvo()) {
        db_home_conectado();
        focus_tab(objects.tv_dashboard, objects.tab_home);
        
    } else {
        db_home_desconectado();

        focus_tab(objects.tv_dashboard, objects.tab_config, false);
        focus_tab(objects.tv_config, objects.tab_wifi);

        // 2. Oculta ou desabilita abas
        lv_obj_t * tab_btns = lv_tabview_get_tab_btns(objects.tv_dashboard);
        lv_btnmatrix_set_btn_ctrl(tab_btns, 1, LV_BTNMATRIX_CTRL_DISABLED); // Desativa Docker (índice 1)
    }

    /* Release the mutex */
    bsp_display_unlock();

    Serial.println(title + " pronto!");
}

void loop()
{
//     Serial.println("IDLE loop");
    delay(1000);
}

static void focus_tab(lv_obj_t * tabview, lv_obj_t * target_page, bool send_event) {
    uint32_t id_tab = lv_obj_get_index(target_page);
    lv_tabview_set_act(tabview, id_tab, LV_ANIM_OFF);
    if(send_event)
        lv_event_send(tabview, LV_EVENT_VALUE_CHANGED, NULL);
}

static void evento_mudanca_aba_cb(lv_event_t * e) {
    lv_obj_t * tabview = lv_event_get_target(e);
    
    uint16_t aba_ativa = lv_tabview_get_tab_act(tabview);
    
    switch (aba_ativa) {
        case 0:
            // O usuário acabou de abrir a aba HOME
            Serial.println("aba OUTRAS aberta. Atualizando dados...");
            lv_textarea_set_text(objects.ed_http, ler_http().c_str());
            lv_textarea_set_text(objects.ed_port, String(ler_port()).c_str());
            break;
            
        case 1:
            // O usuário acabou de abrir a aba CONFIG
            Serial.println("aba WIFI aberta. Escaneando WiFi...");
            tab_config_escanear_redes();
            break;
    }
}