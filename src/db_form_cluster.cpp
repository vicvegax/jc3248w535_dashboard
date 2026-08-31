#include "ui/ui.h"
#include "db_form_cluster.h"
#include "config.h"

extern "C" void action_fechar_form_cluster(lv_event_t *e) {
  // TODO: Implement action cancelar_cluster here
  lv_event_send(objects.keyboard_1, LV_EVENT_CANCEL, NULL);
  lv_scr_load(objects.main);
}


extern "C" void action_salvar_form_cluster(lv_event_t *e) {
  // TODO: Implement action salvar_config here
  // lv_event_send(objects.keyboard_1, LV_EVENT_CANCEL, NULL);
  Cluster item = {-1, String(lv_textarea_get_text(objects.ed_cluster_nome)), String(lv_textarea_get_text(objects.ed_cluster_http)), "abcd1234", String(lv_textarea_get_text(objects.ed_cluster_port)).toInt()};
  adicionarCluster(item);
  Serial.println("Configuração HTTP salva!");
  lv_obj_t * label = lv_obj_get_child(objects.bt_salvar_cluster, 0);
  lv_event_send(objects.keyboard_1, LV_EVENT_CANCEL, NULL);
  lv_scr_load(objects.main);
  Serial.printf("Form Cluster salvo: %s - %s:%d\n", item.nome.c_str(), item.http.c_str(), item.port);
}
