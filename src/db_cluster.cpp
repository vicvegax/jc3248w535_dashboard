#include "ui/ui.h"
#include "db_cluster.h"
#include "config.h"
#include "requisicao.h"
#include <WiFi.h>

extern lv_style_t estilo_checked;

<<<<<<< HEAD
extern "C" void action_del_cluster(lv_event_t *e) {
    // TODO: Implement action del_cluster here
    if(cluster.id >= 0) {
      deletarCluster(cluster.id);
      atualizaClusterGlobal(-1);
      lv_lista_clusters();
    }
=======
namespace {

bool cluster_ui_criada = false;
lv_obj_t *lb_det_nome = nullptr;
lv_obj_t *lb_det_host = nullptr;
lv_obj_t *lb_det_port = nullptr;
lv_obj_t *lb_det_status = nullptr;
lv_obj_t *bt_testar_cluster = nullptr;

void bt_cluster_cb(lv_event_t *e);
void bt_testar_cb(lv_event_t *e);

void atualizar_card_detalhes() {
  if (!cluster_ui_criada) return;

  if (cluster.http == "") {
    lv_label_set_text(lb_det_nome, "Nome: (nenhum)");
    lv_label_set_text(lb_det_host, "Host: --");
    lv_label_set_text(lb_det_port, "Porta: --");
    lv_label_set_text(lb_det_status, "Status: Sem no");
    lv_obj_set_style_text_color(lb_det_status, lv_palette_main(LV_PALETTE_AMBER), 0);
    return;
  }

  lv_label_set_text_fmt(lb_det_nome, "Nome: %s", cluster.nome.c_str());
  lv_label_set_text_fmt(lb_det_host, "Host: %s", cluster.http.c_str());
  lv_label_set_text_fmt(lb_det_port, "Porta: %d", cluster.port);
  lv_label_set_text(lb_det_status, "Status: Aguardando");
  lv_obj_set_style_text_color(lb_det_status, lv_color_hex(0x9AA0A6), 0);
>>>>>>> 4292549 (melhorias:tema,status,alertas,tempo de tela,modal confirmacao)
}

void cluster_ping_ok_cb(JsonDocument &doc) {
  (void)doc;
  if (!cluster_ui_criada) return;
  lv_label_set_text(lb_det_status, "Status: Online");
  lv_obj_set_style_text_color(lb_det_status, lv_palette_main(LV_PALETTE_GREEN), 0);
}

void cluster_ping_erro_cb(int httpCode) {
  if (!cluster_ui_criada) return;
  if (httpCode == -100) {
    lv_label_set_text(lb_det_status, "Status: Sem Wi-Fi");
  } else if (httpCode > 0) {
    lv_label_set_text_fmt(lb_det_status, "Status: HTTP %d", httpCode);
  } else {
    lv_label_set_text(lb_det_status, "Status: Offline");
  }
  lv_obj_set_style_text_color(lb_det_status, lv_palette_main(LV_PALETTE_RED), 0);
}

<<<<<<< HEAD
    lv_obj_t * btn = lv_list_add_btn(objects.ls_clusters, NULL, item.nome.c_str());
    lv_obj_set_user_data(btn, (void*)(uintptr_t)i);
    lv_obj_add_flag(btn, LV_OBJ_FLAG_CHECKABLE);
    lv_obj_add_event_cb(btn, bt_cluster_cb, LV_EVENT_CLICKED, NULL);
    // Ao criar cada botão dentro do 'for', aplique o estilo:
    lv_obj_add_style(btn, &estilo_checked, LV_STATE_CHECKED);
    if(i == cluster.id) {
      lv_obj_add_state(btn, LV_STATE_CHECKED);
    }
=======
void testar_cluster_ativo(bool show_spinner) {
  if (!cluster_ui_criada) return;
  if (cluster.http == "") {
    atualizar_card_detalhes();
    return;
  }
  if (WiFi.status() != WL_CONNECTED) {
    lv_label_set_text(lb_det_status, "Status: Sem Wi-Fi");
    lv_obj_set_style_text_color(lb_det_status, lv_palette_main(LV_PALETTE_AMBER), 0);
    return;
  }

  lv_label_set_text(lb_det_status, "Status: Testando...");
  lv_obj_set_style_text_color(lb_det_status, lv_color_hex(0x9AA0A6), 0);
  get("/", cluster_ping_ok_cb, show_spinner, cluster_ping_erro_cb);
}

void bt_testar_cb(lv_event_t *e) {
  if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
    testar_cluster_ativo(true);
>>>>>>> 4292549 (melhorias:tema,status,alertas,tempo de tela,modal confirmacao)
  }
}

void bt_cluster_cb(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);
  lv_obj_t *btn = lv_event_get_target(e);
  if (code == LV_EVENT_CLICKED) {
    int id = (int)(uintptr_t)lv_obj_get_user_data(btn);

    lv_obj_t *parent = lv_obj_get_parent(btn);
    uint32_t total_itens = lv_obj_get_child_cnt(parent);
    for (uint32_t i = 0; i < total_itens; i++) {
      lv_obj_t *item = lv_obj_get_child(parent, i);
      if (item != btn) {
        lv_obj_clear_state(item, LV_STATE_CHECKED);
      }
    }

    lv_obj_add_state(btn, LV_STATE_CHECKED);
    atualizaClusterGlobal(id);
    atualizar_card_detalhes();
  }
}

} // namespace

void db_cluster_init() {
  if (cluster_ui_criada) return;

  lv_obj_t *tab = objects.tab_cluster;
  lv_obj_clear_state(tab, LV_STATE_DISABLED);
  lv_obj_clear_flag(tab, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_pad_all(tab, 8, 0);

  // Oculta os labels antigos de Temp, CPU e RAM (agora exibidos na Home)
  lv_obj_add_flag(objects.lb_cluster_temp, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(objects.lb_cluster_cpu, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(objects.lb_cluster_ram, LV_OBJ_FLAG_HIDDEN);

  // Reposiciona a lista e o botão Adicionar na coluna esquerda (w=176)
  lv_obj_set_pos(objects.ls_clusters, 0, 22);
  lv_obj_set_size(objects.ls_clusters, 176, 182);

  lv_obj_set_pos(objects.bt_add_cluster, 0, 214);
  lv_obj_set_size(objects.bt_add_cluster, 176, 46);

  // Card de detalhes do nó selecionado na direita (x=186, w=188)
  lv_obj_t *card = lv_obj_create(tab);
  lv_obj_set_pos(card, 186, 22);
  lv_obj_set_size(card, 188, 182);
  lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_bg_color(card, lv_color_hex(0x1E222A), 0);
  lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
  lv_obj_set_style_border_color(card, lv_color_hex(0x323846), 0);
  lv_obj_set_style_border_width(card, 1, 0);
  lv_obj_set_style_radius(card, 10, 0);
  lv_obj_set_style_pad_all(card, 10, 0);

  lv_obj_t *lb_titulo = lv_label_create(card);
  lv_label_set_text(lb_titulo, "No Selecionado");
  lv_obj_set_style_text_font(lb_titulo, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(lb_titulo, lv_color_hex(0xD0D7DE), 0);
  lv_obj_set_pos(lb_titulo, 0, 0);

  lb_det_nome = lv_label_create(card);
  lv_label_set_text(lb_det_nome, "Nome: --");
  lv_label_set_long_mode(lb_det_nome, LV_LABEL_LONG_DOT);
  lv_obj_set_width(lb_det_nome, 166);
  lv_obj_set_style_text_font(lb_det_nome, &lv_font_montserrat_12, 0);
  lv_obj_set_style_text_color(lb_det_nome, lv_color_white(), 0);
  lv_obj_set_pos(lb_det_nome, 0, 32);

  lb_det_host = lv_label_create(card);
  lv_label_set_text(lb_det_host, "Host: --");
  lv_label_set_long_mode(lb_det_host, LV_LABEL_LONG_DOT);
  lv_obj_set_width(lb_det_host, 166);
  lv_obj_set_style_text_font(lb_det_host, &lv_font_montserrat_12, 0);
  lv_obj_set_style_text_color(lb_det_host, lv_color_hex(0x9AA0A6), 0);
  lv_obj_set_pos(lb_det_host, 0, 62);

  lb_det_port = lv_label_create(card);
  lv_label_set_text(lb_det_port, "Porta: --");
  lv_obj_set_style_text_font(lb_det_port, &lv_font_montserrat_12, 0);
  lv_obj_set_style_text_color(lb_det_port, lv_color_hex(0x9AA0A6), 0);
  lv_obj_set_pos(lb_det_port, 0, 92);

  lb_det_status = lv_label_create(card);
  lv_label_set_text(lb_det_status, "Status: --");
  lv_obj_set_style_text_font(lb_det_status, &lv_font_montserrat_12, 0);
  lv_obj_set_style_text_color(lb_det_status, lv_color_hex(0x9AA0A6), 0);
  lv_obj_set_pos(lb_det_status, 0, 128);

  // Botões Testar e Remover abaixo do card
  bt_testar_cluster = lv_btn_create(tab);
  lv_obj_set_pos(bt_testar_cluster, 186, 214);
  lv_obj_set_size(bt_testar_cluster, 89, 46);
  lv_obj_add_event_cb(bt_testar_cluster, bt_testar_cb, LV_EVENT_CLICKED, NULL);
  lv_obj_t *lb_testar = lv_label_create(bt_testar_cluster);
  lv_label_set_text(lb_testar, "Testar");
  lv_obj_center(lb_testar);

  lv_obj_set_pos(objects.bt_del_cluster, 285, 214);
  lv_obj_set_size(objects.bt_del_cluster, 89, 46);
  lv_obj_set_style_bg_color(objects.bt_del_cluster, lv_palette_main(LV_PALETTE_RED), 0);

  cluster_ui_criada = true;
  atualizar_card_detalhes();
}

static void executar_remocao_cluster() {
  int ativo = obterClusterAtivoIndex();
  Cluster itens[MAX_CLUSTERS];
  int total = carregarClusters(itens);
  if (total <= 0 || ativo < 0 || ativo >= total) return;

  deletarCluster(ativo);
  int novo_total = total - 1;
  int novo_ativo = (novo_total > 0) ? ((ativo >= novo_total) ? novo_total - 1 : ativo) : -1;
  atualizaClusterGlobal(novo_ativo);
  lv_lista_clusters();
}

extern "C" void action_del_cluster(lv_event_t *e) {
  (void)e;
  int ativo = obterClusterAtivoIndex();
  Cluster itens[MAX_CLUSTERS];
  int total = carregarClusters(itens);
  if (total <= 0 || ativo < 0 || ativo >= total) return;

  char msg[96];
  snprintf(msg, sizeof(msg), "Deseja remover o no '%s'?", itens[ativo].nome.c_str());
  exibir_confirmacao("Remover Cluster", msg, "Remover", executar_remocao_cluster);
}

extern "C" void action_show_form_cluster(lv_event_t *e) {
  (void)e;
  lv_textarea_set_text(objects.ed_cluster_nome, "");
  lv_textarea_set_text(objects.ed_cluster_http, "");
  lv_textarea_set_text(objects.ed_cluster_port, "3000");
  lv_scr_load(objects.form_cluster);
}

void lv_lista_clusters() {
  if (!cluster_ui_criada) {
    db_cluster_init();
  }

  Cluster itens[MAX_CLUSTERS];
  lv_obj_clean(objects.ls_clusters);
  int total = carregarClusters(itens);
  int ativo = obterClusterAtivoIndex();
  if (total > 0 && (ativo < 0 || ativo >= total)) {
    ativo = 0;
    atualizaClusterGlobal(0);
  }

  for (int i = 0; i < total; i++) {
    Cluster item = itens[i];
    lv_obj_t *btn = lv_list_add_btn(objects.ls_clusters, NULL, item.nome.c_str());
    lv_obj_set_user_data(btn, (void *)(uintptr_t)i);
    lv_obj_add_flag(btn, LV_OBJ_FLAG_CHECKABLE);
    lv_obj_add_event_cb(btn, bt_cluster_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_style(btn, &estilo_checked, LV_STATE_CHECKED);

    if (i == ativo) {
      lv_obj_add_state(btn, LV_STATE_CHECKED);
    }
  }

  atualizar_card_detalhes();
}
