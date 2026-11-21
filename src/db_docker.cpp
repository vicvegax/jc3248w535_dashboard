#include "ui/ui.h"
#include "db_docker.h"
#include "requisicao.h"
#include "config.h"
#include <WiFi.h>
#include <cstdlib>
#include <cstring>

extern lv_style_t estilo_checked;

namespace {

struct ContItemData {
  char id[24];
  char name[48];
  bool ativo;
};

bool docker_ui_criada = false;

lv_obj_t *lb_docker_resumo = nullptr;
lv_obj_t *bt_docker_refresh = nullptr;

lv_obj_t *lb_det_nome = nullptr;
lv_obj_t *lb_det_id = nullptr;
lv_obj_t *lb_det_estado = nullptr;

lv_obj_t *lb_det_cpu_val = nullptr;
lv_obj_t *bar_det_cpu = nullptr;
lv_obj_t *lb_det_cpu_sub = nullptr;

lv_obj_t *lb_det_ram_val = nullptr;
lv_obj_t *bar_det_ram = nullptr;
lv_obj_t *lb_det_ram_sub = nullptr;

lv_color_t cor_por_percentual(float pct) {
  if (pct >= 85.0f) return lv_palette_main(LV_PALETTE_RED);
  if (pct >= 60.0f) return lv_palette_main(LV_PALETTE_AMBER);
  return lv_palette_main(LV_PALETTE_GREEN);
}

void limpar_lista_containers() {
  uint32_t itens_atuais = lv_obj_get_child_cnt(objects.ls_cont);
  for (uint32_t i = 0; i < itens_atuais; i++) {
    lv_obj_t *item = lv_obj_get_child(objects.ls_cont, i);
    void *user_data = lv_obj_get_user_data(item);
    if (user_data != NULL) {
      free(user_data);
      lv_obj_set_user_data(item, NULL);
    }
  }
  lv_obj_clean(objects.ls_cont);
}

void resetar_card_detalhes(const char *titulo, const char *estado_txt, lv_color_t cor_estado) {
  if (!docker_ui_criada) return;
  lv_label_set_text(lb_det_nome, titulo);
  lv_label_set_text(lb_det_id, "ID: --");
  lv_label_set_text(lb_det_estado, estado_txt);
  lv_obj_set_style_text_color(lb_det_estado, cor_estado, 0);

  lv_label_set_text(lb_det_cpu_val, "CPU: --%");
  lv_bar_set_value(bar_det_cpu, 0, LV_ANIM_OFF);
  lv_label_set_text(lb_det_cpu_sub, "CPUs: --");

  lv_label_set_text(lb_det_ram_val, "RAM: --");
  lv_bar_set_value(bar_det_ram, 0, LV_ANIM_OFF);
  lv_label_set_text(lb_det_ram_sub, "Lim: --");
}

void cont_stat_cb(JsonDocument &doc) {
  if (!docker_ui_criada) return;

  const char *cpu_pct_str = doc["cpu"]["percent"] | "0%";
  int cpu_online = doc["cpu"]["onlineCpus"] | 0;
  const char *ram_usage_str = doc["ram"]["usageMB"] | "0 MB";
  const char *ram_limit_str = doc["ram"]["limitMB"] | "0 MB";
  const char *ram_pct_str = doc["ram"]["percent"] | "0%";

  float cpu_pct = strtof(cpu_pct_str, nullptr);
  if (cpu_pct < 0.0f) cpu_pct = 0.0f;
  if (cpu_pct > 100.0f) cpu_pct = 100.0f;

  float ram_pct = strtof(ram_pct_str, nullptr);
  if (ram_pct < 0.0f) ram_pct = 0.0f;
  if (ram_pct > 100.0f) ram_pct = 100.0f;

  lv_label_set_text(lb_det_estado, "RUNNING");
  lv_obj_set_style_text_color(lb_det_estado, lv_palette_main(LV_PALETTE_GREEN), 0);

  lv_label_set_text_fmt(lb_det_cpu_val, "CPU: %s", cpu_pct_str);
  lv_bar_set_value(bar_det_cpu, static_cast<int>(cpu_pct + 0.5f), LV_ANIM_OFF);
  lv_obj_set_style_bg_color(bar_det_cpu, cor_por_percentual(cpu_pct), LV_PART_INDICATOR);
  lv_label_set_text_fmt(lb_det_cpu_sub, "CPUs online: %d", cpu_online);

  lv_label_set_text_fmt(lb_det_ram_val, "RAM: %s (%s)", ram_usage_str, ram_pct_str);
  lv_bar_set_value(bar_det_ram, static_cast<int>(ram_pct + 0.5f), LV_ANIM_OFF);
  lv_obj_set_style_bg_color(bar_det_ram, cor_por_percentual(ram_pct), LV_PART_INDICATOR);
  lv_label_set_text_fmt(lb_det_ram_sub, "Limite: %s", ram_limit_str);
}

void cont_stat_erro_cb(int httpCode) {
  if (!docker_ui_criada) return;
  if (httpCode == -100) {
    lv_label_set_text(lb_det_estado, "SEM WI-FI");
  } else if (httpCode > 0) {
    lv_label_set_text_fmt(lb_det_estado, "ERRO HTTP %d", httpCode);
  } else {
    lv_label_set_text(lb_det_estado, "ERRO STATS");
  }
  lv_obj_set_style_text_color(lb_det_estado, lv_palette_main(LV_PALETTE_RED), 0);
}

void bt_cont_cb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;

  lv_obj_t *obj = lv_event_get_target(e);
  uint32_t total_itens = lv_obj_get_child_cnt(objects.ls_cont);
  for (uint32_t i = 0; i < total_itens; i++) {
    lv_obj_t *filho = lv_obj_get_child(objects.ls_cont, i);
    if (filho != obj) {
      lv_obj_clear_state(filho, LV_STATE_CHECKED);
    }
  }
  lv_obj_add_state(obj, LV_STATE_CHECKED);

  ContItemData *data = static_cast<ContItemData *>(lv_obj_get_user_data(obj));
  if (data == nullptr) return;

  lv_label_set_text(lb_det_nome, data->name);
  lv_label_set_text_fmt(lb_det_id, "ID: %s", data->id);

  if (data->ativo) {
    lv_label_set_text(lb_det_estado, "Carregando...");
    lv_obj_set_style_text_color(lb_det_estado, lv_color_hex(0x9AA0A6), 0);
    get("/container/" + String(data->id), cont_stat_cb, true, cont_stat_erro_cb);
  } else {
    lv_label_set_text(lb_det_estado, "STOPPED");
    lv_obj_set_style_text_color(lb_det_estado, lv_palette_main(LV_PALETTE_RED), 0);
    lv_label_set_text(lb_det_cpu_val, "CPU: 0%");
    lv_bar_set_value(bar_det_cpu, 0, LV_ANIM_OFF);
    lv_label_set_text(lb_det_cpu_sub, "Container parado");
    lv_label_set_text(lb_det_ram_val, "RAM: 0 MB");
    lv_bar_set_value(bar_det_ram, 0, LV_ANIM_OFF);
    lv_label_set_text(lb_det_ram_sub, "Sem consumo");
  }
}

void adicionar_item_lista(const char *name, const char *id, bool ativo) {
  ContItemData *data = static_cast<ContItemData *>(malloc(sizeof(ContItemData)));
  if (data == nullptr) return;

  snprintf(data->id, sizeof(data->id), "%s", id);
  snprintf(data->name, sizeof(data->name), "%s", name);
  data->ativo = ativo;

  const char *icone = ativo ? LV_SYMBOL_PLAY : LV_SYMBOL_STOP;
  lv_obj_t *btn = lv_list_add_btn(objects.ls_cont, icone, data->name);
  lv_obj_set_user_data(btn, data);
  lv_obj_add_flag(btn, LV_OBJ_FLAG_CHECKABLE);
  lv_obj_add_event_cb(btn, bt_cont_cb, LV_EVENT_CLICKED, NULL);
  lv_obj_add_style(btn, &estilo_checked, LV_STATE_CHECKED);
}

void lv_lista_dockers_cb(JsonDocument &doc) {
  if (!docker_ui_criada) return;

  JsonArray ativos = doc["ativos"].as<JsonArray>();
  JsonArray inativos = doc["inativos"].as<JsonArray>();

  int qtd_ativos = ativos.size();
  int qtd_inativos = inativos.size();
  lv_label_set_text_fmt(lb_docker_resumo, "Containers (%d ativos / %d inat.)", qtd_ativos, qtd_inativos);

  lv_list_add_text(objects.ls_cont, "Ativos");
  for (JsonVariant container : ativos) {
    const char *name = container["name"].as<const char *>();
    const char *id = container["id"].as<const char *>();
    if (name && id) {
      adicionar_item_lista(name, id, true);
    }
  }

  lv_list_add_text(objects.ls_cont, "Inativos");
  for (JsonVariant container : inativos) {
    const char *name = container["name"].as<const char *>();
    const char *id = container["id"].as<const char *>();
    if (name && id) {
      adicionar_item_lista(name, id, false);
    }
  }

  resetar_card_detalhes("Selecione um container", "PRONTO", lv_color_hex(0x9AA0A6));
}

void lv_lista_dockers_erro_cb(int httpCode) {
  if (!docker_ui_criada) return;
  lv_label_set_text(lb_docker_resumo, "Containers (erro)");
  if (httpCode == -100) {
    resetar_card_detalhes("Sem conexao Wi-Fi", "SEM WI-FI", lv_palette_main(LV_PALETTE_AMBER));
  } else if (httpCode > 0) {
    char msg[24];
    snprintf(msg, sizeof(msg), "HTTP %d", httpCode);
    resetar_card_detalhes("Falha ao listar", msg, lv_palette_main(LV_PALETTE_RED));
  } else {
    resetar_card_detalhes("API indisponivel", "OFFLINE", lv_palette_main(LV_PALETTE_RED));
  }
}

void bt_refresh_cb(lv_event_t *e) {
  if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
    lv_lista_containers();
  }
}

} // namespace

void db_docker_init() {
  if (docker_ui_criada) return;

  lv_obj_t *tab = objects.tab_docker;
  lv_obj_clear_flag(tab, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_pad_all(tab, 8, 0);

  // Oculta os elementos legados do EEZ Studio nessa aba (mantendo ls_cont)
  uint32_t total_filhos = lv_obj_get_child_cnt(tab);
  for (uint32_t i = 0; i < total_filhos; i++) {
    lv_obj_t *filho = lv_obj_get_child(tab, i);
    if (filho != objects.ls_cont) {
      lv_obj_add_flag(filho, LV_OBJ_FLAG_HIDDEN);
    }
  }

  // Cabeçalho: resumo + botão atualizar
  lb_docker_resumo = lv_label_create(tab);
  lv_label_set_text(lb_docker_resumo, "Containers");
  lv_obj_set_style_text_font(lb_docker_resumo, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(lb_docker_resumo, lv_color_hex(0xD0D7DE), 0);
  lv_obj_set_pos(lb_docker_resumo, 0, 4);

  bt_docker_refresh = lv_btn_create(tab);
  lv_obj_set_size(bt_docker_refresh, 92, 26);
  lv_obj_set_pos(bt_docker_refresh, 282, 0);
  lv_obj_add_event_cb(bt_docker_refresh, bt_refresh_cb, LV_EVENT_CLICKED, NULL);
  lv_obj_t *lb_ref = lv_label_create(bt_docker_refresh);
  lv_label_set_text(lb_ref, LV_SYMBOL_REFRESH " Atual.");
  lv_obj_set_style_text_font(lb_ref, &lv_font_montserrat_12, 0);
  lv_obj_center(lb_ref);

  // Lista na esquerda (w=180, h=236)
  lv_obj_set_pos(objects.ls_cont, 0, 32);
  lv_obj_set_size(objects.ls_cont, 180, 236);

  // Card de detalhes na direita (x=188, w=186, h=236)
  lv_obj_t *card = lv_obj_create(tab);
  lv_obj_set_pos(card, 188, 32);
  lv_obj_set_size(card, 186, 236);
  lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_bg_color(card, lv_color_hex(0x1E222A), 0);
  lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
  lv_obj_set_style_border_color(card, lv_color_hex(0x323846), 0);
  lv_obj_set_style_border_width(card, 1, 0);
  lv_obj_set_style_radius(card, 10, 0);
  lv_obj_set_style_pad_all(card, 10, 0);

  lb_det_nome = lv_label_create(card);
  lv_label_set_text(lb_det_nome, "Selecione um container");
  lv_label_set_long_mode(lb_det_nome, LV_LABEL_LONG_DOT);
  lv_obj_set_width(lb_det_nome, 164);
  lv_obj_set_style_text_font(lb_det_nome, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(lb_det_nome, lv_color_white(), 0);
  lv_obj_set_pos(lb_det_nome, 0, 0);

  lb_det_id = lv_label_create(card);
  lv_label_set_text(lb_det_id, "ID: --");
  lv_obj_set_style_text_font(lb_det_id, &lv_font_montserrat_10, 0);
  lv_obj_set_style_text_color(lb_det_id, lv_color_hex(0x8B949E), 0);
  lv_obj_set_pos(lb_det_id, 0, 22);

  lb_det_estado = lv_label_create(card);
  lv_label_set_text(lb_det_estado, "PRONTO");
  lv_obj_set_style_text_font(lb_det_estado, &lv_font_montserrat_12, 0);
  lv_obj_set_style_text_color(lb_det_estado, lv_color_hex(0x9AA0A6), 0);
  lv_obj_set_pos(lb_det_estado, 0, 42);

  // Seção CPU
  lb_det_cpu_val = lv_label_create(card);
  lv_label_set_text(lb_det_cpu_val, "CPU: --%");
  lv_obj_set_style_text_font(lb_det_cpu_val, &lv_font_montserrat_12, 0);
  lv_obj_set_style_text_color(lb_det_cpu_val, lv_color_hex(0xD0D7DE), 0);
  lv_obj_set_pos(lb_det_cpu_val, 0, 72);

  bar_det_cpu = lv_bar_create(card);
  lv_obj_set_size(bar_det_cpu, 164, 10);
  lv_obj_set_pos(bar_det_cpu, 0, 92);
  lv_bar_set_range(bar_det_cpu, 0, 100);
  lv_bar_set_value(bar_det_cpu, 0, LV_ANIM_OFF);
  lv_obj_set_style_bg_color(bar_det_cpu, lv_color_hex(0x2D3340), LV_PART_MAIN);
  lv_obj_set_style_bg_color(bar_det_cpu, lv_palette_main(LV_PALETTE_GREEN), LV_PART_INDICATOR);

  lb_det_cpu_sub = lv_label_create(card);
  lv_label_set_text(lb_det_cpu_sub, "CPUs: --");
  lv_obj_set_style_text_font(lb_det_cpu_sub, &lv_font_montserrat_10, 0);
  lv_obj_set_style_text_color(lb_det_cpu_sub, lv_color_hex(0x8B949E), 0);
  lv_obj_set_pos(lb_det_cpu_sub, 0, 106);

  // Seção RAM
  lb_det_ram_val = lv_label_create(card);
  lv_label_set_text(lb_det_ram_val, "RAM: --");
  lv_label_set_long_mode(lb_det_ram_val, LV_LABEL_LONG_DOT);
  lv_obj_set_width(lb_det_ram_val, 164);
  lv_obj_set_style_text_font(lb_det_ram_val, &lv_font_montserrat_12, 0);
  lv_obj_set_style_text_color(lb_det_ram_val, lv_color_hex(0xD0D7DE), 0);
  lv_obj_set_pos(lb_det_ram_val, 0, 136);

  bar_det_ram = lv_bar_create(card);
  lv_obj_set_size(bar_det_ram, 164, 10);
  lv_obj_set_pos(bar_det_ram, 0, 156);
  lv_bar_set_range(bar_det_ram, 0, 100);
  lv_bar_set_value(bar_det_ram, 0, LV_ANIM_OFF);
  lv_obj_set_style_bg_color(bar_det_ram, lv_color_hex(0x2D3340), LV_PART_MAIN);
  lv_obj_set_style_bg_color(bar_det_ram, lv_palette_main(LV_PALETTE_GREEN), LV_PART_INDICATOR);

  lb_det_ram_sub = lv_label_create(card);
  lv_label_set_text(lb_det_ram_sub, "Lim: --");
  lv_label_set_long_mode(lb_det_ram_sub, LV_LABEL_LONG_DOT);
  lv_obj_set_width(lb_det_ram_sub, 164);
  lv_obj_set_style_text_font(lb_det_ram_sub, &lv_font_montserrat_10, 0);
  lv_obj_set_style_text_color(lb_det_ram_sub, lv_color_hex(0x8B949E), 0);
  lv_obj_set_pos(lb_det_ram_sub, 0, 170);

  docker_ui_criada = true;
}

void lv_lista_containers() {
  if (!docker_ui_criada) {
    db_docker_init();
  }

  limpar_lista_containers();

  if (cluster.http == "") {
    lv_label_set_text(lb_docker_resumo, "Containers (sem cluster)");
    resetar_card_detalhes("Selecione um cluster", "SEM CLUSTER", lv_palette_main(LV_PALETTE_AMBER));
    return;
  }

  lv_label_set_text(lb_docker_resumo, "Containers (carregando...)");
  resetar_card_detalhes("Carregando lista...", "AGUARDE", lv_color_hex(0x9AA0A6));
  get("/containers", lv_lista_dockers_cb, true, lv_lista_dockers_erro_cb);
}
