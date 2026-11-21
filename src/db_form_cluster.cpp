#include "ui/ui.h"
#include "db_form_cluster.h"
#include "db_cluster.h"
#include "config.h"

extern "C" void action_fechar_form_cluster(lv_event_t *e) {
  (void)e;
  lv_event_send(objects.keyboard_1, LV_EVENT_CANCEL, NULL);
  lv_scr_load(objects.main);
}

extern "C" void action_salvar_form_cluster(lv_event_t *e) {
  (void)e;
  lv_event_send(objects.keyboard_1, LV_EVENT_CANCEL, NULL);

  String nome = String(lv_textarea_get_text(objects.ed_cluster_nome));
  String http = String(lv_textarea_get_text(objects.ed_cluster_http));
  int port = String(lv_textarea_get_text(objects.ed_cluster_port)).toInt();
  nome.trim();
  http.trim();
  if (http.length() == 0) {
    return;
  }
  if (nome.length() == 0) {
    nome = http;
  }
  if (port <= 0) {
    port = 3000;
  }

  Cluster item = {nome, http, "abcd1234", port};
  adicionarCluster(item);

  Cluster itens[MAX_CLUSTERS];
  int total = carregarClusters(itens);
  if (total > 0) {
    atualizaClusterGlobal(total - 1);
  }

  lv_scr_load(objects.main);
  lv_lista_clusters();
}
