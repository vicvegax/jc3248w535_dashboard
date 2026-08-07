#include "ui/ui.h"
#include "db_cluster.h"
#include "config.h"

static void bt_cluster_cb(lv_event_t *e);
extern lv_style_t estilo_checked;

extern "C" void action_del_cluster(lv_event_t *e) {
    // TODO: Implement action del_cluster here
}

extern "C" void action_show_form_cluster(lv_event_t *e) {
    // TODO: Implement action show_add_cluster here
    lv_scr_load(objects.form_cluster);
}

void lista_clusters() {
  Serial.println("CLUSTER aberta. Listando nos...");
  Cluster itens[MAX_CLUSTERS];
  lv_obj_clean(objects.ls_clusters); // Limpa a lista antes de adicionar novos itens
  int total = carregarClusters(itens);
  for(int i = 0; i < total; i++) {
    Cluster item = itens[i];

    lv_obj_t * btn = lv_list_add_btn(objects.ls_clusters, NULL, item.nome.c_str());
    lv_obj_set_user_data(btn, (void*)(uintptr_t)i);
    lv_obj_add_flag(btn, LV_OBJ_FLAG_CHECKABLE);
    lv_obj_add_event_cb(btn, bt_cluster_cb, LV_EVENT_CLICKED, NULL);
    // Ao criar cada botão dentro do 'for', aplique o estilo:
    lv_obj_add_style(btn, &estilo_checked, LV_STATE_CHECKED);
  }
}

static void bt_cluster_cb(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);
  lv_obj_t * btn = lv_event_get_target(e);
  if(code == LV_EVENT_CLICKED) {
    int id = (int)(uintptr_t)lv_obj_get_user_data(btn);
    Serial.println("Cluster selecionado: " + String(id));
    
    lv_obj_t * parent = lv_obj_get_parent(btn);
    uint32_t total_itens = lv_obj_get_child_cnt(parent);
    for(uint32_t i = 0; i < total_itens; i++) {
        lv_obj_t * item = lv_obj_get_child(parent, i);
        
        // Se o filho for um botão e NÃO for o que acabou de ser clicado, desmarca ele
        if(item != btn) {
          lv_obj_clear_state(item, LV_STATE_CHECKED);
        }
    }

    lv_obj_add_state(btn, LV_STATE_CHECKED);
    atualizaClusterGlobal(id);
  }
}