#include "ui/ui.h"
#include "db_alertas.h"
#include "requisicao.h"
#include "config.h"
#include <esp_bsp.h>
#include <WiFi.h>
#include <time.h>
#include <cstdlib>
#include <cstring>

namespace {

constexpr float LIMITE_CPU_WARN = 70.0f;
constexpr float LIMITE_CPU_CRIT = 85.0f;
constexpr float LIMITE_RAM_WARN = 75.0f;
constexpr float LIMITE_RAM_CRIT = 85.0f;
constexpr float LIMITE_TEMP_WARN = 65.0f;
constexpr float LIMITE_TEMP_CRIT = 75.0f;
constexpr int MAX_HISTORICO = 12;

struct EventoAlerta {
  char data_hora[20];
  char nivel[24];
  char mensagem[128];
  lv_color_t cor;
};

struct EstadoCategoria {
  bool ativo;
  char chave[64];
  char data_hora[20];
};

bool alertas_ui_criada = false;
bool alertas_show_spinner = true;
int total_alertas_ativos = 0;
EventoAlerta historico[MAX_HISTORICO] = {};
int historico_qtd = 0;
EstadoCategoria estados[ALERTA_CAT_MAX] = {};

lv_obj_t *lb_alertas_resumo = nullptr;
lv_obj_t *lista_alertas = nullptr;

void obter_data_hora_str(char *buf, size_t len) {
  struct tm timeinfo;
  if (getLocalTime(&timeinfo, 0)) {
    snprintf(buf, len, "%02d/%02d %02d:%02d:%02d",
             timeinfo.tm_mday,
             timeinfo.tm_mon + 1,
             timeinfo.tm_hour,
             timeinfo.tm_min,
             timeinfo.tm_sec);
  } else {
    unsigned long seg = millis() / 1000UL;
    unsigned long min = seg / 60UL;
    unsigned long s = seg % 60UL;
    snprintf(buf, len, "up %lum%02lus", min, s);
  }
}

void criar_card_alerta(const char *nivel, const char *data_hora, lv_color_t cor, const char *mensagem) {
  lv_obj_t *card = lv_obj_create(lista_alertas);
  lv_obj_set_width(card, lv_pct(100));
  lv_obj_set_height(card, LV_SIZE_CONTENT);
  lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_bg_color(card, lv_color_hex(0x1E222A), 0);
  lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
  lv_obj_set_style_border_color(card, cor, 0);
  lv_obj_set_style_border_width(card, 1, 0);
  lv_obj_set_style_radius(card, 6, 0);
  lv_obj_set_style_pad_all(card, 8, 0);

  lv_obj_t *lb_lvl = lv_label_create(card);
  lv_label_set_text(lb_lvl, nivel);
  lv_obj_set_style_text_font(lb_lvl, &lv_font_montserrat_12, 0);
  lv_obj_set_style_text_color(lb_lvl, cor, 0);
  lv_obj_set_pos(lb_lvl, 0, 0);

  if (data_hora && strlen(data_hora) > 0) {
    lv_obj_t *lb_dt = lv_label_create(card);
    lv_label_set_text(lb_dt, data_hora);
    lv_obj_set_style_text_font(lb_dt, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(lb_dt, lv_color_hex(0x8B949E), 0);
    lv_obj_align(lb_dt, LV_ALIGN_TOP_RIGHT, 0, 1);
  }

  lv_obj_t *lb_msg = lv_label_create(card);
  lv_label_set_text(lb_msg, mensagem);
  lv_label_set_long_mode(lb_msg, LV_LABEL_LONG_WRAP);
  lv_obj_set_width(lb_msg, 330);
  lv_obj_set_style_text_font(lb_msg, &lv_font_montserrat_12, 0);
  lv_obj_set_style_text_color(lb_msg, lv_color_white(), 0);
  lv_obj_set_pos(lb_msg, 0, 18);
}

void registrar_no_historico(const char *data_hora, const char *nivel, lv_color_t cor, const char *mensagem) {
  int limite = (historico_qtd < MAX_HISTORICO) ? historico_qtd : MAX_HISTORICO - 1;
  for (int i = limite; i > 0; i--) {
    historico[i] = historico[i - 1];
  }

  snprintf(historico[0].data_hora, sizeof(historico[0].data_hora), "%s", data_hora);
  snprintf(historico[0].nivel, sizeof(historico[0].nivel), "%s", nivel);
  snprintf(historico[0].mensagem, sizeof(historico[0].mensagem), "%s", mensagem);
  historico[0].cor = cor;

  if (historico_qtd < MAX_HISTORICO) {
    historico_qtd++;
  }
}

void adicionar_alerta_ativo(AlertaCategoria cat,
                            const char *chave_estado,
                            const char *nivel,
                            lv_color_t cor,
                            const char *mensagem) {
  db_alertas_atualizar_estado(cat, true, chave_estado, nivel, cor, mensagem);
  criar_card_alerta(nivel, estados[cat].data_hora, cor, mensagem);
  total_alertas_ativos++;
}

void finalizar_resumo_alertas() {
  if (total_alertas_ativos == 0) {
    char agora[20];
    obter_data_hora_str(agora, sizeof(agora));
    lv_label_set_text(lb_alertas_resumo, "Tudo OK");
    lv_obj_set_style_text_color(lb_alertas_resumo, lv_palette_main(LV_PALETTE_GREEN), 0);
    criar_card_alerta("OK", agora, lv_palette_main(LV_PALETTE_GREEN),
                      "Nenhum alerta ativo. Sistema operando dentro dos parametros.");
  } else {
    lv_label_set_text_fmt(lb_alertas_resumo, "%d alerta(s)", total_alertas_ativos);
    lv_obj_set_style_text_color(lb_alertas_resumo, lv_palette_main(LV_PALETTE_RED), 0);
  }

  if (historico_qtd > 0) {
    lv_obj_t *lb_hist = lv_label_create(lista_alertas);
    lv_label_set_text(lb_hist, "Historico de eventos:");
    lv_obj_set_style_text_font(lb_hist, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(lb_hist, lv_color_hex(0x8B949E), 0);

    for (int i = 0; i < historico_qtd; i++) {
      criar_card_alerta(historico[i].nivel, historico[i].data_hora, historico[i].cor, historico[i].mensagem);
    }
  }
}

void alertas_containers_cb(JsonDocument &doc) {
  if (!alertas_ui_criada) return;

  JsonArray inativos = doc["inativos"].as<JsonArray>();
  int qtd_inativos = inativos.size();
  if (qtd_inativos > 0) {
    String nomes = "";
    int count = 0;
    for (JsonVariant c : inativos) {
      const char *name = c["name"] | "sem-nome";
      if (count > 0) nomes += ", ";
      nomes += name;
      count++;
      if (count >= 3) break;
    }
    if (qtd_inativos > 3) {
      nomes += " ...";
    }
    char chave[64];
    snprintf(chave, sizeof(chave), "%d:%s", qtd_inativos, nomes.c_str());
    char msg[128];
    snprintf(msg, sizeof(msg), "%d container(s) parado(s): %s", qtd_inativos, nomes.c_str());
    adicionar_alerta_ativo(ALERTA_CAT_DOCKER, chave, "DOCKER - ATENCAO", lv_palette_main(LV_PALETTE_AMBER), msg);
  } else {
    db_alertas_atualizar_estado(ALERTA_CAT_DOCKER, false, "", "", lv_color_black(), "");
  }

  finalizar_resumo_alertas();
}

void alertas_containers_erro_cb(int httpCode) {
  if (!alertas_ui_criada) return;
  char chave[24];
  snprintf(chave, sizeof(chave), "ERR:%d", httpCode);
  char msg[64];
  snprintf(msg, sizeof(msg), "Falha ao consultar containers (HTTP %d).", httpCode);
  adicionar_alerta_ativo(ALERTA_CAT_DOCKER, chave, "DOCKER - ERRO", lv_palette_main(LV_PALETTE_RED), msg);
  finalizar_resumo_alertas();
}

void alertas_system_cb(JsonDocument &doc) {
  if (!alertas_ui_criada) return;

  lv_obj_clean(lista_alertas);
  total_alertas_ativos = 0;
  db_alertas_atualizar_estado(ALERTA_CAT_API, false, "", "", lv_color_black(), "");

  // CPU
  float cpu_pct = doc["cpu"]["globalUsagePercent"] | 0.0f;
  if (cpu_pct >= LIMITE_CPU_CRIT) {
    char msg[80];
    snprintf(msg, sizeof(msg), "Uso de CPU critico: %.1f%% (limite %.0f%%)", cpu_pct, LIMITE_CPU_CRIT);
    adicionar_alerta_ativo(ALERTA_CAT_CPU, "CRIT", "CPU - CRITICO", lv_palette_main(LV_PALETTE_RED), msg);
  } else if (cpu_pct >= LIMITE_CPU_WARN) {
    char msg[64];
    snprintf(msg, sizeof(msg), "Uso de CPU elevado: %.1f%%", cpu_pct);
    adicionar_alerta_ativo(ALERTA_CAT_CPU, "WARN", "CPU - ATENCAO", lv_palette_main(LV_PALETTE_AMBER), msg);
  } else {
    db_alertas_atualizar_estado(ALERTA_CAT_CPU, false, "", "", lv_color_black(), "");
  }

  // RAM
  float ram_used = strtof(doc["memory"]["usedGB"] | "0", nullptr);
  float ram_total = strtof(doc["memory"]["totalGB"] | "0", nullptr);
  float ram_pct = (ram_total > 0.0f) ? (ram_used / ram_total) * 100.0f : 0.0f;
  if (ram_pct >= LIMITE_RAM_CRIT) {
    char msg[88];
    snprintf(msg, sizeof(msg), "Uso de RAM critico: %.1f%% (%.1f/%.1f GB)", ram_pct, ram_used, ram_total);
    adicionar_alerta_ativo(ALERTA_CAT_RAM, "CRIT", "RAM - CRITICO", lv_palette_main(LV_PALETTE_RED), msg);
  } else if (ram_pct >= LIMITE_RAM_WARN) {
    char msg[88];
    snprintf(msg, sizeof(msg), "Uso de RAM elevado: %.1f%% (%.1f/%.1f GB)", ram_pct, ram_used, ram_total);
    adicionar_alerta_ativo(ALERTA_CAT_RAM, "WARN", "RAM - ATENCAO", lv_palette_main(LV_PALETTE_AMBER), msg);
  } else {
    db_alertas_atualizar_estado(ALERTA_CAT_RAM, false, "", "", lv_color_black(), "");
  }

  // Temperatura
  const char *temp_str = doc["temp"]["main"] | "NA";
  if (strcmp(temp_str, "NA") != 0 && strlen(temp_str) > 0) {
    float temp_val = strtof(temp_str, nullptr);
    if (temp_val >= LIMITE_TEMP_CRIT) {
      char msg[64];
      snprintf(msg, sizeof(msg), "Temperatura critica: %s (limite %.0f°C)", temp_str, LIMITE_TEMP_CRIT);
      adicionar_alerta_ativo(ALERTA_CAT_TEMP, "CRIT", "TEMP - CRITICO", lv_palette_main(LV_PALETTE_RED), msg);
    } else if (temp_val >= LIMITE_TEMP_WARN) {
      char msg[64];
      snprintf(msg, sizeof(msg), "Temperatura elevada: %s", temp_str);
      adicionar_alerta_ativo(ALERTA_CAT_TEMP, "WARN", "TEMP - ATENCAO", lv_palette_main(LV_PALETTE_AMBER), msg);
    } else {
      db_alertas_atualizar_estado(ALERTA_CAT_TEMP, false, "", "", lv_color_black(), "");
    }
  } else {
    db_alertas_atualizar_estado(ALERTA_CAT_TEMP, false, "", "", lv_color_black(), "");
  }

  get("/containers", alertas_containers_cb, alertas_show_spinner, alertas_containers_erro_cb);
}

void alertas_system_erro_cb(int httpCode) {
  if (!alertas_ui_criada) return;
  lv_obj_clean(lista_alertas);
  total_alertas_ativos = 0;
  char chave[24];
  snprintf(chave, sizeof(chave), "ERR:%d", httpCode);
  if (httpCode == -100) {
    adicionar_alerta_ativo(ALERTA_CAT_API, chave, "REDE - ERRO", lv_palette_main(LV_PALETTE_RED), "ESP32 sem conexao Wi-Fi.");
  } else {
    char msg[80];
    snprintf(msg, sizeof(msg), "API do no %s nao respondeu (HTTP %d).", cluster.http.c_str(), httpCode);
    adicionar_alerta_ativo(ALERTA_CAT_API, chave, "API - OFFLINE", lv_palette_main(LV_PALETTE_RED), msg);
  }
  finalizar_resumo_alertas();
}

void bt_alertas_refresh_cb(lv_event_t *e) {
  if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
    lv_info_alertas();
  }
}

} // namespace

void db_alertas_atualizar_estado(AlertaCategoria cat,
                                 bool ativo,
                                 const char *chave_estado,
                                 const char *nivel,
                                 lv_color_t cor,
                                 const char *mensagem) {
  if (cat >= ALERTA_CAT_MAX) return;

  EstadoCategoria &st = estados[cat];
  if (!ativo) {
    st.ativo = false;
    st.chave[0] = '\0';
    return;
  }

  if (!chave_estado) chave_estado = "";
  if (st.ativo && strcmp(st.chave, chave_estado) == 0) {
    // Mesmo estado já registrado; não repete no histórico
    return;
  }

  st.ativo = true;
  snprintf(st.chave, sizeof(st.chave), "%s", chave_estado);
  obter_data_hora_str(st.data_hora, sizeof(st.data_hora));
  registrar_no_historico(st.data_hora, nivel, cor, mensagem);
}

void db_alertas_init() {
  if (alertas_ui_criada) return;

  lv_obj_t *tab = objects.tab_alertas;
  lv_obj_clear_flag(tab, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_pad_all(tab, 8, 0);

  lv_obj_t *lb_titulo = lv_label_create(tab);
  lv_label_set_text(lb_titulo, "Alertas");
  lv_obj_set_style_text_font(lb_titulo, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(lb_titulo, lv_color_hex(0xD0D7DE), 0);
  lv_obj_set_pos(lb_titulo, 0, 4);

  lb_alertas_resumo = lv_label_create(tab);
  lv_label_set_text(lb_alertas_resumo, "Aguardando...");
  lv_obj_set_style_text_font(lb_alertas_resumo, &lv_font_montserrat_12, 0);
  lv_obj_set_style_text_color(lb_alertas_resumo, lv_color_hex(0x8B949E), 0);
  lv_obj_set_pos(lb_alertas_resumo, 68, 6);

  lv_obj_t *bt_refresh = lv_btn_create(tab);
  lv_obj_set_size(bt_refresh, 96, 26);
  lv_obj_set_pos(bt_refresh, 278, 0);
  lv_obj_add_event_cb(bt_refresh, bt_alertas_refresh_cb, LV_EVENT_CLICKED, NULL);
  lv_obj_t *lb_ref = lv_label_create(bt_refresh);
  lv_label_set_text(lb_ref, LV_SYMBOL_REFRESH " Verif.");
  lv_obj_set_style_text_font(lb_ref, &lv_font_montserrat_12, 0);
  lv_obj_center(lb_ref);

  lista_alertas = lv_obj_create(tab);
  lv_obj_set_pos(lista_alertas, 0, 34);
  lv_obj_set_size(lista_alertas, 374, 210);
  lv_obj_set_style_bg_opa(lista_alertas, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(lista_alertas, 0, 0);
  lv_obj_set_style_pad_all(lista_alertas, 0, 0);
  lv_obj_set_flex_flow(lista_alertas, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_pad_row(lista_alertas, 8, 0);

  lv_obj_t *lb_limites = lv_label_create(tab);
  lv_label_set_text(lb_limites, "Limites: CPU >=85% | RAM >=85% | Temp >=75C | Docker parados");
  lv_obj_set_style_text_font(lb_limites, &lv_font_montserrat_10, 0);
  lv_obj_set_style_text_color(lb_limites, lv_color_hex(0x8B949E), 0);
  lv_obj_set_pos(lb_limites, 0, 252);

  alertas_ui_criada = true;
}

void lv_info_alertas(bool show_spinner) {
  if (!alertas_ui_criada) {
    db_alertas_init();
  }

  if (cluster.http == "") {
    lv_obj_clean(lista_alertas);
    total_alertas_ativos = 0;
    lv_label_set_text(lb_alertas_resumo, "Sem cluster");
    lv_obj_set_style_text_color(lb_alertas_resumo, lv_palette_main(LV_PALETTE_AMBER), 0);
    criar_card_alerta("CONFIG", "", lv_palette_main(LV_PALETTE_AMBER), "Nenhum cluster selecionado.");
    return;
  }

  alertas_show_spinner = show_spinner;
  if (show_spinner) {
    lv_obj_clean(lista_alertas);
    total_alertas_ativos = 0;
    lv_label_set_text(lb_alertas_resumo, "Verificando...");
    lv_obj_set_style_text_color(lb_alertas_resumo, lv_color_hex(0x9AA0A6), 0);
  }
  get("/system", alertas_system_cb, show_spinner, alertas_system_erro_cb);
}
