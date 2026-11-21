#include <Arduino.h>
#include <WiFi.h>
#include <cstdlib>
#include <cstring>
#include "ui/ui.h"
#include "db_home.h"
#include "db_alertas.h"
#include "requisicao.h"
#include "config.h"
#include <esp_bsp.h>

namespace {

constexpr uint32_t HOME_REFRESH_INTERVAL_MS = 5000;
constexpr uint32_t HOME_CHECK_INTERVAL_MS = 500;
constexpr uint32_t HOME_STALE_TIMEOUT_MS = 12000;
constexpr int MAX_CORE_LABELS = 8;

constexpr float LIMITE_CPU_WARN = 70.0f;
constexpr float LIMITE_CPU_CRIT = 85.0f;
constexpr float LIMITE_RAM_WARN = 75.0f;
constexpr float LIMITE_RAM_CRIT = 85.0f;
constexpr float LIMITE_TEMP_WARN = 65.0f;
constexpr float LIMITE_TEMP_CRIT = 75.0f;

bool home_ui_criada = false;
uint32_t ultimo_tick_check_ms = 0;
uint32_t ultimo_request_home_ms = 0;
uint32_t ultimo_sucesso_home_ms = 0;
bool aguardando_primeira_resposta = false;
float temp_max_observada = -1000.0f;

bool home_sys_critico = false;
bool home_sys_atencao = false;
String home_sys_resumo = "";

lv_obj_t *bar_cpu = nullptr;
lv_obj_t *lb_cpu_val = nullptr;

lv_obj_t *bar_ram = nullptr;
lv_obj_t *lb_ram_val = nullptr;
lv_obj_t *lb_ram_sub = nullptr;

lv_obj_t *lb_temp_val = nullptr;
lv_obj_t *bar_temp = nullptr;
lv_obj_t *lb_temp_sub = nullptr;

lv_obj_t *lb_home_status = nullptr;
lv_obj_t *lb_home_alerta = nullptr;

enum ModoDetalheHome {
  DETALHE_CPU = 0,
  DETALHE_RAM = 1
};

ModoDetalheHome modo_detalhe_atual = DETALHE_CPU;
lv_obj_t *panel_detalhe = nullptr;
lv_obj_t *lb_detalhe_hdr = nullptr;
lv_obj_t *cont_cores = nullptr;
lv_obj_t *lb_cores[MAX_CORE_LABELS] = {nullptr};
lv_obj_t *lb_top_titulo = nullptr;
lv_obj_t *lb_top_itens[5] = {nullptr};

lv_color_t cor_por_percentual(float pct) {
  if (pct >= 85.0f) return lv_palette_main(LV_PALETTE_RED);
  if (pct >= 60.0f) return lv_palette_main(LV_PALETTE_AMBER);
  return lv_palette_main(LV_PALETTE_GREEN);
}

lv_color_t cor_por_temperatura(float temp_c) {
  if (temp_c >= 75.0f) return lv_palette_main(LV_PALETTE_RED);
  if (temp_c >= 60.0f) return lv_palette_main(LV_PALETTE_AMBER);
  return lv_palette_main(LV_PALETTE_GREEN);
}

lv_obj_t *criar_card(lv_obj_t *parent, lv_coord_t x, lv_coord_t y, lv_coord_t w, lv_coord_t h) {
  lv_obj_t *card = lv_obj_create(parent);
  lv_obj_set_pos(card, x, y);
  lv_obj_set_size(card, w, h);
  lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_bg_color(card, lv_color_hex(0x1E222A), 0);
  lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
  lv_obj_set_style_border_color(card, lv_color_hex(0x323846), 0);
  lv_obj_set_style_border_width(card, 1, 0);
  lv_obj_set_style_radius(card, 8, 0);
  lv_obj_set_style_pad_all(card, 8, 0);
  return card;
}

lv_obj_t *criar_barra_metrica(lv_obj_t *parent) {
  lv_obj_t *bar = lv_bar_create(parent);
  lv_obj_set_size(bar, 92, 12);
  lv_bar_set_range(bar, 0, 100);
  lv_bar_set_value(bar, 0, LV_ANIM_OFF);
  lv_obj_set_style_bg_color(bar, lv_color_hex(0x2D3340), LV_PART_MAIN);
  lv_obj_set_style_bg_color(bar, lv_palette_main(LV_PALETTE_GREEN), LV_PART_INDICATOR);
  lv_obj_align(bar, LV_ALIGN_CENTER, 0, 26);
  return bar;
}

void top_containers_cb(JsonDocument &doc) {
  if (!home_ui_criada || !panel_detalhe) return;

  JsonArray arr = (modo_detalhe_atual == DETALHE_CPU)
                      ? doc["topCpu"].as<JsonArray>()
                      : doc["topRam"].as<JsonArray>();

  int idx = 0;
  for (JsonVariant item : arr) {
    if (idx >= 5) break;
    const char *name = item["name"] | "sem-nome";
    char buf[128];
    if (modo_detalhe_atual == DETALHE_CPU) {
      const char *pct = item["percent"] | "0%";
      float val = strtof(pct, nullptr);
      const char *cor_uso = (val >= 85.0f) ? "F44336" : ((val >= 60.0f) ? "FFC107" : "4FC3F7");
      snprintf(buf, sizeof(buf), "#D0D7DE %d. %s#  #%s %s#", idx + 1, name, cor_uso, pct);
    } else {
      const char *usage = item["usageMB"] | "0 MB";
      const char *pct = item["percent"] | "0%";
      float val = strtof(pct, nullptr);
      const char *cor_uso = (val >= 85.0f) ? "F44336" : ((val >= 60.0f) ? "FFC107" : "4DB6AC");
      snprintf(buf, sizeof(buf), "#D0D7DE %d. %s#  #%s %s (%s)#", idx + 1, name, cor_uso, usage, pct);
    }
    lv_label_set_text(lb_top_itens[idx], buf);
    lv_obj_set_style_text_color(lb_top_itens[idx], lv_color_hex(0xD0D7DE), 0);
    lv_obj_clear_flag(lb_top_itens[idx], LV_OBJ_FLAG_HIDDEN);
    idx++;
  }

  if (idx == 0) {
    lv_label_set_text(lb_top_itens[0], "Nenhum container ativo.");
    lv_obj_set_style_text_color(lb_top_itens[0], lv_color_hex(0x8B949E), 0);
    lv_obj_clear_flag(lb_top_itens[0], LV_OBJ_FLAG_HIDDEN);
    idx = 1;
  }

  for (int i = idx; i < 5; i++) {
    lv_obj_add_flag(lb_top_itens[i], LV_OBJ_FLAG_HIDDEN);
  }
}

void top_containers_erro_cb(int httpCode) {
  if (!home_ui_criada || !panel_detalhe) return;
  char msg[64];
  if (httpCode == 404) {
    snprintf(msg, sizeof(msg), "Rota /containers/top nao encontrada na API.");
  } else {
    snprintf(msg, sizeof(msg), "Erro ao carregar Top 5 (HTTP %d).", httpCode);
  }
  lv_label_set_text(lb_top_itens[0], msg);
  lv_obj_set_style_text_color(lb_top_itens[0], lv_palette_main(LV_PALETTE_RED), 0);
  lv_obj_clear_flag(lb_top_itens[0], LV_OBJ_FLAG_HIDDEN);
  for (int i = 1; i < 5; i++) {
    lv_obj_add_flag(lb_top_itens[i], LV_OBJ_FLAG_HIDDEN);
  }
}

void abrir_detalhe_home(ModoDetalheHome modo) {
  if (!panel_detalhe) return;
  modo_detalhe_atual = modo;

  if (modo == DETALHE_CPU) {
    lv_label_set_text(lb_detalhe_hdr, "CPU - Cores & Top 5");
    lv_label_set_text(lb_top_titulo, "Top 5 Containers por uso de CPU:");
    lv_obj_clear_flag(cont_cores, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_pos(lb_top_titulo, 0, 92);
    for (int i = 0; i < 5; i++) {
      lv_obj_set_pos(lb_top_itens[i], 0, 112 + (i * 22));
    }
  } else {
    lv_label_set_text(lb_detalhe_hdr, "RAM - Top 5 Containers");
    lv_label_set_text(lb_top_titulo, "Top 5 Containers por uso de RAM:");
    lv_obj_add_flag(cont_cores, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_pos(lb_top_titulo, 0, 42);
    for (int i = 0; i < 5; i++) {
      lv_obj_set_pos(lb_top_itens[i], 0, 66 + (i * 28));
    }
  }

  lv_label_set_text(lb_top_itens[0], "Carregando Top 5...");
  lv_obj_set_style_text_color(lb_top_itens[0], lv_color_hex(0x9AA0A6), 0);
  lv_obj_clear_flag(lb_top_itens[0], LV_OBJ_FLAG_HIDDEN);
  for (int i = 1; i < 5; i++) {
    lv_obj_add_flag(lb_top_itens[i], LV_OBJ_FLAG_HIDDEN);
  }

  lv_obj_clear_flag(panel_detalhe, LV_OBJ_FLAG_HIDDEN);
  lv_obj_move_foreground(panel_detalhe);

  if (cluster.http != "") {
    get("/containers/top", top_containers_cb, true, top_containers_erro_cb);
  } else {
    lv_label_set_text(lb_top_itens[0], "Nenhum cluster selecionado.");
  }
}

void card_cpu_cb(lv_event_t *e) {
  if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
    abrir_detalhe_home(DETALHE_CPU);
  }
}

void card_ram_cb(lv_event_t *e) {
  if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
    abrir_detalhe_home(DETALHE_RAM);
  }
}

void fechar_detalhe_cb(lv_event_t *e) {
  if (lv_event_get_code(e) == LV_EVENT_CLICKED && panel_detalhe) {
    lv_obj_add_flag(panel_detalhe, LV_OBJ_FLAG_HIDDEN);
  }
}

void card_alerta_cb(lv_event_t *e) {
  if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
    lv_tabview_set_act(objects.tv_dashboard, 4, LV_ANIM_OFF);
    lv_info_alertas();
  }
}

void atualizar_texto_alerta_home(bool has_docker_alert, const String &docker_msg) {
  bool critico = home_sys_critico;
  bool atencao = home_sys_atencao || has_docker_alert;

  String resumo = home_sys_resumo;
  if (has_docker_alert) {
    if (resumo.length() > 0) resumo += " | ";
    resumo += docker_msg;
  }

  if (critico) {
    String txt = "CRITICO: " + resumo + " (toque p/ detalhes)";
    lv_label_set_text(lb_home_alerta, txt.c_str());
    lv_obj_set_style_text_color(lb_home_alerta, lv_palette_main(LV_PALETTE_RED), 0);
  } else if (atencao) {
    String txt = "ATENCAO: " + resumo + " (toque p/ detalhes)";
    lv_label_set_text(lb_home_alerta, txt.c_str());
    lv_obj_set_style_text_color(lb_home_alerta, lv_palette_main(LV_PALETTE_AMBER), 0);
  } else {
    lv_label_set_text(lb_home_alerta, "Tudo OK - Sistema operando dentro dos parametros.");
    lv_obj_set_style_text_color(lb_home_alerta, lv_palette_main(LV_PALETTE_GREEN), 0);
  }
}

void home_containers_cb(JsonDocument &doc) {
  if (!home_ui_criada) return;

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
      if (count >= 2) break;
    }
    if (qtd_inativos > 2) {
      nomes += " ...";
    }
    char chave[64];
    snprintf(chave, sizeof(chave), "%d:%s", qtd_inativos, nomes.c_str());
    char msg[96];
    snprintf(msg, sizeof(msg), "%d cont. parado(s): %s", qtd_inativos, nomes.c_str());
    db_alertas_atualizar_estado(ALERTA_CAT_DOCKER, true, chave, "DOCKER - ATENCAO", lv_palette_main(LV_PALETTE_AMBER), msg);
    atualizar_texto_alerta_home(true, String(msg));
  } else {
    db_alertas_atualizar_estado(ALERTA_CAT_DOCKER, false, "", "", lv_color_black(), "");
    atualizar_texto_alerta_home(false, "");
  }
}

void home_containers_erro_cb(int httpCode) {
  if (!home_ui_criada) return;
  char msg[48];
  snprintf(msg, sizeof(msg), "Docker erro HTTP %d", httpCode);
  atualizar_texto_alerta_home(true, String(msg));
}

void home_system_cb(JsonDocument &doc) {
  if (!home_ui_criada) return;

  ultimo_sucesso_home_ms = millis();
  aguardando_primeira_resposta = false;

  // --- CPU ---
  float cpu_pct = doc["cpu"]["globalUsagePercent"] | 0.0f;
  if (cpu_pct < 0.0f) cpu_pct = 0.0f;
  if (cpu_pct > 100.0f) cpu_pct = 100.0f;
  lv_color_t cor_cpu = cor_por_percentual(cpu_pct);

  lv_bar_set_value(bar_cpu, static_cast<int>(cpu_pct + 0.5f), LV_ANIM_OFF);
  lv_obj_set_style_bg_color(bar_cpu, cor_cpu, LV_PART_INDICATOR);
  lv_label_set_text_fmt(lb_cpu_val, "%d%%", static_cast<int>(cpu_pct + 0.5f));
  lv_obj_set_style_text_color(lb_cpu_val, cor_cpu, 0);

  // --- RAM ---
  float ram_used = strtof(doc["memory"]["usedGB"] | "0", nullptr);
  float ram_total = strtof(doc["memory"]["totalGB"] | "0", nullptr);
  float ram_free = ram_total - ram_used;
  if (ram_free < 0.0f) ram_free = 0.0f;
  float ram_pct = (ram_total > 0.0f) ? (ram_used / ram_total) * 100.0f : 0.0f;
  if (ram_pct < 0.0f) ram_pct = 0.0f;
  if (ram_pct > 100.0f) ram_pct = 100.0f;
  lv_color_t cor_ram = cor_por_percentual(ram_pct);

  lv_bar_set_value(bar_ram, static_cast<int>(ram_pct + 0.5f), LV_ANIM_OFF);
  lv_obj_set_style_bg_color(bar_ram, cor_ram, LV_PART_INDICATOR);
  lv_label_set_text_fmt(lb_ram_val, "%d%%", static_cast<int>(ram_pct + 0.5f));
  lv_obj_set_style_text_color(lb_ram_val, cor_ram, 0);
  char ram_buf[48];
  snprintf(ram_buf, sizeof(ram_buf), "Uso: %.1f | Liv: %.1f\nTot: %.1f GB", ram_used, ram_free, ram_total);
  lv_label_set_text(lb_ram_sub, ram_buf);

  // --- Temperatura ---
  const char *temp_main_str = doc["temp"]["main"] | "NA";
  const char *temp_max_str = doc["temp"]["max"] | "NA";
  float temp_val = strtof(temp_main_str, nullptr);
  bool temp_valida = (strcmp(temp_main_str, "NA") != 0 && strlen(temp_main_str) > 0);

  if (temp_valida) {
    if (temp_val > temp_max_observada) {
      temp_max_observada = temp_val;
    }
    if (strcmp(temp_max_str, "NA") != 0 && strlen(temp_max_str) > 0) {
      float api_max = strtof(temp_max_str, nullptr);
      if (api_max > temp_max_observada) {
        temp_max_observada = api_max;
      }
    }

    lv_color_t cor_temp = cor_por_temperatura(temp_val);
    lv_label_set_text(lb_temp_val, temp_main_str);
    lv_obj_set_style_text_color(lb_temp_val, cor_temp, 0);
    int bar_val = static_cast<int>(temp_val + 0.5f);
    if (bar_val < 0) bar_val = 0;
    if (bar_val > 100) bar_val = 100;
    lv_bar_set_value(bar_temp, bar_val, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(bar_temp, cor_temp, LV_PART_INDICATOR);
    char temp_buf[24];
    snprintf(temp_buf, sizeof(temp_buf), "Max: %.1f°C", temp_max_observada);
    lv_label_set_text(lb_temp_sub, temp_buf);
  } else {
    lv_label_set_text(lb_temp_val, "NA");
    lv_obj_set_style_text_color(lb_temp_val, lv_color_hex(0x9AA0A6), 0);
    lv_bar_set_value(bar_temp, 0, LV_ANIM_OFF);
    lv_label_set_text(lb_temp_sub, "Max: NA");
  }

  // --- Cores da CPU (painel ao clicar no card CPU) ---
  JsonArray cores = doc["cpu"]["cores"].as<JsonArray>();
  int idx = 0;
  for (JsonVariant core : cores) {
    if (idx >= MAX_CORE_LABELS) break;
    float c_pct = core.as<float>();
    lv_label_set_text_fmt(lb_cores[idx], "Core %d: %d%%", idx, static_cast<int>(c_pct + 0.5f));
    lv_obj_set_style_text_color(lb_cores[idx], cor_por_percentual(c_pct), 0);
    lv_obj_clear_flag(lb_cores[idx], LV_OBJ_FLAG_HIDDEN);
    idx++;
  }
  for (int i = idx; i < MAX_CORE_LABELS; i++) {
    lv_obj_add_flag(lb_cores[i], LV_OBJ_FLAG_HIDDEN);
  }

  // --- Resumo de Alertas na Faixa Inferior ---
  bool critico = false;
  bool atencao = false;
  String resumo = "";

  auto append_alerta = [&](const char *parte) {
    if (resumo.length() > 0) resumo += " | ";
    resumo += parte;
  };

  if (cpu_pct >= LIMITE_CPU_CRIT) {
    critico = true;
    char buf[24];
    snprintf(buf, sizeof(buf), "CPU %.0f%%", cpu_pct);
    append_alerta(buf);
    char log_msg[64];
    snprintf(log_msg, sizeof(log_msg), "Uso de CPU critico: %.1f%%", cpu_pct);
    db_alertas_atualizar_estado(ALERTA_CAT_CPU, true, "CRIT", "CPU - CRITICO", lv_palette_main(LV_PALETTE_RED), log_msg);
  } else if (cpu_pct >= LIMITE_CPU_WARN) {
    atencao = true;
    char buf[24];
    snprintf(buf, sizeof(buf), "CPU %.0f%%", cpu_pct);
    append_alerta(buf);
    char log_msg[64];
    snprintf(log_msg, sizeof(log_msg), "Uso de CPU elevado: %.1f%%", cpu_pct);
    db_alertas_atualizar_estado(ALERTA_CAT_CPU, true, "WARN", "CPU - ATENCAO", lv_palette_main(LV_PALETTE_AMBER), log_msg);
  } else {
    db_alertas_atualizar_estado(ALERTA_CAT_CPU, false, "", "", lv_color_black(), "");
  }

  if (ram_pct >= LIMITE_RAM_CRIT) {
    critico = true;
    char buf[24];
    snprintf(buf, sizeof(buf), "RAM %.0f%%", ram_pct);
    append_alerta(buf);
    char log_msg[64];
    snprintf(log_msg, sizeof(log_msg), "Uso de RAM critico: %.1f%%", ram_pct);
    db_alertas_atualizar_estado(ALERTA_CAT_RAM, true, "CRIT", "RAM - CRITICO", lv_palette_main(LV_PALETTE_RED), log_msg);
  } else if (ram_pct >= LIMITE_RAM_WARN) {
    atencao = true;
    char buf[24];
    snprintf(buf, sizeof(buf), "RAM %.0f%%", ram_pct);
    append_alerta(buf);
    char log_msg[64];
    snprintf(log_msg, sizeof(log_msg), "Uso de RAM elevado: %.1f%%", ram_pct);
    db_alertas_atualizar_estado(ALERTA_CAT_RAM, true, "WARN", "RAM - ATENCAO", lv_palette_main(LV_PALETTE_AMBER), log_msg);
  } else {
    db_alertas_atualizar_estado(ALERTA_CAT_RAM, false, "", "", lv_color_black(), "");
  }

  if (temp_valida) {
    if (temp_val >= LIMITE_TEMP_CRIT) {
      critico = true;
      char buf[24];
      snprintf(buf, sizeof(buf), "Temp %.0f°C", temp_val);
      append_alerta(buf);
      char log_msg[64];
      snprintf(log_msg, sizeof(log_msg), "Temperatura critica: %s", temp_main_str);
      db_alertas_atualizar_estado(ALERTA_CAT_TEMP, true, "CRIT", "TEMP - CRITICO", lv_palette_main(LV_PALETTE_RED), log_msg);
    } else if (temp_val >= LIMITE_TEMP_WARN) {
      atencao = true;
      char buf[24];
      snprintf(buf, sizeof(buf), "Temp %.0f°C", temp_val);
      append_alerta(buf);
      char log_msg[64];
      snprintf(log_msg, sizeof(log_msg), "Temperatura elevada: %s", temp_main_str);
      db_alertas_atualizar_estado(ALERTA_CAT_TEMP, true, "WARN", "TEMP - ATENCAO", lv_palette_main(LV_PALETTE_AMBER), log_msg);
    } else {
      db_alertas_atualizar_estado(ALERTA_CAT_TEMP, false, "", "", lv_color_black(), "");
    }
  } else {
    db_alertas_atualizar_estado(ALERTA_CAT_TEMP, false, "", "", lv_color_black(), "");
  }

  home_sys_critico = critico;
  home_sys_atencao = atencao;
  home_sys_resumo = resumo;

  // --- Status ---
  lv_label_set_text(lb_home_status, "Online");
  lv_obj_set_style_text_color(lb_home_status, lv_palette_main(LV_PALETTE_GREEN), 0);

  // Encadeia verificação de containers parados para refletir na Home também
  get("/containers", home_containers_cb, false, home_containers_erro_cb);
}

} // namespace

void db_conectado() {
  // String meu_ip = "Conectado como " + WiFi.localIP().toString();
String meu_ip = "IP: " + WiFi.localIP().toString();
  lv_label_set_text(objects.lb_conectado, meu_ip.c_str());
}

void db_desconectado() {
  lv_label_set_text(objects.lb_conectado, "* DESCONECTADO *");
}

void db_home_init() {
  if (home_ui_criada) return;

  lv_obj_t *home = objects.tab_home;
  lv_obj_clear_flag(home, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_pad_all(home, 8, 0);

  constexpr lv_coord_t CARD_W = 118;
  constexpr lv_coord_t CARD_H = 176;
  constexpr lv_coord_t GAP = 10;

  // --- Card CPU (clicável para abrir detalhamento de cores) ---
  lv_obj_t *card_cpu = criar_card(home, 0, 0, CARD_W, CARD_H);
  lv_obj_add_flag(card_cpu, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(card_cpu, card_cpu_cb, LV_EVENT_CLICKED, NULL);

  lv_obj_t *lb_cpu_title = lv_label_create(card_cpu);
  lv_label_set_text(lb_cpu_title, "CPU");
  lv_obj_set_style_text_font(lb_cpu_title, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(lb_cpu_title, lv_color_hex(0xD0D7DE), 0);
  lv_obj_align(lb_cpu_title, LV_ALIGN_TOP_MID, 0, 0);

  lb_cpu_val = lv_label_create(card_cpu);
  lv_label_set_text(lb_cpu_val, "--%");
  lv_obj_set_style_text_font(lb_cpu_val, &lv_font_montserrat_28, 0);
  lv_obj_set_style_text_color(lb_cpu_val, lv_color_white(), 0);
  lv_obj_align(lb_cpu_val, LV_ALIGN_CENTER, 0, -12);

  bar_cpu = criar_barra_metrica(card_cpu);

  lv_obj_t *lb_cpu_sub = lv_label_create(card_cpu);
  lv_label_set_text(lb_cpu_sub, "Toque p/ cores");
  lv_obj_set_style_text_font(lb_cpu_sub, &lv_font_montserrat_10, 0);
  lv_obj_set_style_text_color(lb_cpu_sub, lv_color_hex(0x8B949E), 0);
  lv_obj_align(lb_cpu_sub, LV_ALIGN_BOTTOM_MID, 0, 0);

  // --- Card RAM (clicável para abrir Top 5 RAM) ---
  lv_obj_t *card_ram = criar_card(home, CARD_W + GAP, 0, CARD_W, CARD_H);
  lv_obj_add_flag(card_ram, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(card_ram, card_ram_cb, LV_EVENT_CLICKED, NULL);
  lv_obj_t *lb_ram_title = lv_label_create(card_ram);
  lv_label_set_text(lb_ram_title, "RAM");
  lv_obj_set_style_text_font(lb_ram_title, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(lb_ram_title, lv_color_hex(0xD0D7DE), 0);
  lv_obj_align(lb_ram_title, LV_ALIGN_TOP_MID, 0, 0);

  lb_ram_val = lv_label_create(card_ram);
  lv_label_set_text(lb_ram_val, "--%");
  lv_obj_set_style_text_font(lb_ram_val, &lv_font_montserrat_28, 0);
  lv_obj_set_style_text_color(lb_ram_val, lv_color_white(), 0);
  lv_obj_align(lb_ram_val, LV_ALIGN_CENTER, 0, -20);

  bar_ram = criar_barra_metrica(card_ram);
  lv_obj_align(bar_ram, LV_ALIGN_CENTER, 0, 12);

  lb_ram_sub = lv_label_create(card_ram);
  lv_label_set_text(lb_ram_sub, "Uso: -- | Liv: --\nTot: -- GB");
  lv_obj_set_style_text_font(lb_ram_sub, &lv_font_montserrat_10, 0);
  lv_obj_set_style_text_color(lb_ram_sub, lv_color_hex(0x8B949E), 0);
  lv_obj_set_style_text_align(lb_ram_sub, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_align(lb_ram_sub, LV_ALIGN_BOTTOM_MID, 0, 0);

  // --- Card Temperatura ---
  lv_obj_t *card_temp = criar_card(home, (CARD_W + GAP) * 2, 0, CARD_W, CARD_H);
  lv_obj_t *lb_temp_title = lv_label_create(card_temp);
  lv_label_set_text(lb_temp_title, "TEMP");
  lv_obj_set_style_text_font(lb_temp_title, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(lb_temp_title, lv_color_hex(0xD0D7DE), 0);
  lv_obj_align(lb_temp_title, LV_ALIGN_TOP_MID, 0, 0);

  lb_temp_val = lv_label_create(card_temp);
  lv_label_set_text(lb_temp_val, "--°C");
  lv_obj_set_style_text_font(lb_temp_val, &lv_font_montserrat_26, 0);
  lv_obj_set_style_text_color(lb_temp_val, lv_color_white(), 0);
  lv_obj_align(lb_temp_val, LV_ALIGN_CENTER, 0, -12);

  bar_temp = criar_barra_metrica(card_temp);

  lb_temp_sub = lv_label_create(card_temp);
  lv_label_set_text(lb_temp_sub, "Max: --°C");
  lv_obj_set_style_text_font(lb_temp_sub, &lv_font_montserrat_12, 0);
  lv_obj_set_style_text_color(lb_temp_sub, lv_color_hex(0x8B949E), 0);
  lv_obj_align(lb_temp_sub, LV_ALIGN_BOTTOM_MID, 0, 0);

  // --- Faixa Inferior: Resumo de Alertas + Status (clicável para aba Alertas) ---
  lv_obj_t *card_bottom = criar_card(home, 0, CARD_H + 10, (CARD_W * 3) + (GAP * 2), 82);
  lv_obj_add_flag(card_bottom, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(card_bottom, card_alerta_cb, LV_EVENT_CLICKED, NULL);

  lv_obj_t *lb_alerta_title = lv_label_create(card_bottom);
  lv_label_set_text(lb_alerta_title, "Status do Sistema");
  lv_obj_set_style_text_font(lb_alerta_title, &lv_font_montserrat_12, 0);
  lv_obj_set_style_text_color(lb_alerta_title, lv_color_hex(0x8B949E), 0);
  lv_obj_align(lb_alerta_title, LV_ALIGN_TOP_LEFT, 0, 0);

  lb_home_status = lv_label_create(card_bottom);
  lv_label_set_text(lb_home_status, "Aguardando...");
  lv_obj_set_style_text_font(lb_home_status, &lv_font_montserrat_12, 0);
  lv_obj_set_style_text_color(lb_home_status, lv_color_hex(0x8B949E), 0);
  lv_obj_align(lb_home_status, LV_ALIGN_TOP_RIGHT, 0, 0);

  lb_home_alerta = lv_label_create(card_bottom);
  lv_label_set_text(lb_home_alerta, "Aguardando primeira leitura do no...");
  lv_label_set_long_mode(lb_home_alerta, LV_LABEL_LONG_WRAP);
  lv_obj_set_width(lb_home_alerta, 356);
  lv_obj_set_style_text_font(lb_home_alerta, &lv_font_montserrat_12, 0);
  lv_obj_set_style_text_color(lb_home_alerta, lv_color_hex(0xD0D7DE), 0);
  lv_obj_set_pos(lb_home_alerta, 0, 24);

  // --- Painel Flutuante de Detalhes (CPU Cores + Top 5 / RAM Top 5) ---
  panel_detalhe = lv_obj_create(home);
  lv_obj_set_pos(panel_detalhe, 0, 0);
  lv_obj_set_size(panel_detalhe, 374, 268);
  lv_obj_clear_flag(panel_detalhe, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(panel_detalhe, LV_OBJ_FLAG_HIDDEN);
  lv_obj_set_style_bg_color(panel_detalhe, lv_color_hex(0x181B22), 0);
  lv_obj_set_style_bg_opa(panel_detalhe, LV_OPA_COVER, 0);
  lv_obj_set_style_border_color(panel_detalhe, lv_palette_main(LV_PALETTE_BLUE), 0);
  lv_obj_set_style_border_width(panel_detalhe, 1, 0);
  lv_obj_set_style_radius(panel_detalhe, 10, 0);
  lv_obj_set_style_pad_all(panel_detalhe, 12, 0);

  lb_detalhe_hdr = lv_label_create(panel_detalhe);
  lv_label_set_text(lb_detalhe_hdr, "Detalhes");
  lv_obj_set_style_text_font(lb_detalhe_hdr, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(lb_detalhe_hdr, lv_color_white(), 0);
  lv_obj_set_pos(lb_detalhe_hdr, 0, 4);

  lv_obj_t *bt_fechar = lv_btn_create(panel_detalhe);
  lv_obj_set_size(bt_fechar, 80, 28);
  lv_obj_align(bt_fechar, LV_ALIGN_TOP_RIGHT, 0, 0);
  lv_obj_add_event_cb(bt_fechar, fechar_detalhe_cb, LV_EVENT_CLICKED, NULL);
  lv_obj_t *lb_fechar = lv_label_create(bt_fechar);
  lv_label_set_text(lb_fechar, "Fechar");
  lv_obj_set_style_text_font(lb_fechar, &lv_font_montserrat_12, 0);
  lv_obj_center(lb_fechar);

  cont_cores = lv_obj_create(panel_detalhe);
  lv_obj_set_pos(cont_cores, 0, 36);
  lv_obj_set_size(cont_cores, 348, 48);
  lv_obj_clear_flag(cont_cores, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_bg_opa(cont_cores, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(cont_cores, 0, 0);
  lv_obj_set_style_pad_all(cont_cores, 0, 0);
  lv_obj_set_flex_flow(cont_cores, LV_FLEX_FLOW_ROW_WRAP);
  lv_obj_set_flex_align(cont_cores, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
  lv_obj_set_style_pad_column(cont_cores, 12, 0);
  lv_obj_set_style_pad_row(cont_cores, 4, 0);

  for (int i = 0; i < MAX_CORE_LABELS; i++) {
    lb_cores[i] = lv_label_create(cont_cores);
    lv_label_set_text_fmt(lb_cores[i], "Core %d: --%%", i);
    lv_obj_set_width(lb_cores[i], 78);
    lv_obj_set_style_text_font(lb_cores[i], &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(lb_cores[i], lv_color_hex(0xD0D7DE), 0);
    if (i >= 4) {
      lv_obj_add_flag(lb_cores[i], LV_OBJ_FLAG_HIDDEN);
    }
  }

  lb_top_titulo = lv_label_create(panel_detalhe);
  lv_label_set_text(lb_top_titulo, "Top 5 Containers:");
  lv_obj_set_style_text_font(lb_top_titulo, &lv_font_montserrat_12, 0);
  lv_obj_set_style_text_color(lb_top_titulo, lv_color_hex(0x8B949E), 0);
  lv_obj_set_pos(lb_top_titulo, 0, 92);

  for (int i = 0; i < 5; i++) {
    lb_top_itens[i] = lv_label_create(panel_detalhe);
    lv_label_set_recolor(lb_top_itens[i], true);
    lv_label_set_text(lb_top_itens[i], "");
    lv_label_set_long_mode(lb_top_itens[i], LV_LABEL_LONG_DOT);
    lv_obj_set_width(lb_top_itens[i], 348);
    lv_obj_set_style_text_font(lb_top_itens[i], &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(lb_top_itens[i], lv_color_hex(0xD0D7DE), 0);
    lv_obj_set_pos(lb_top_itens[i], 0, 112 + (i * 22));
  }

  home_ui_criada = true;
}

void lv_info_home(bool show_spinner) {
  if (!home_ui_criada) {
    db_home_init();
  }
  if (cluster.http == "") {
    lv_label_set_text(lb_home_status, "Sem cluster");
    lv_obj_set_style_text_color(lb_home_status, lv_palette_main(LV_PALETTE_AMBER), 0);
    lv_label_set_text(lb_home_alerta, "Nenhum cluster selecionado. Configure na aba Cluster.");
    lv_obj_set_style_text_color(lb_home_alerta, lv_palette_main(LV_PALETTE_AMBER), 0);
    return;
  }

  ultimo_request_home_ms = millis();
  if (ultimo_sucesso_home_ms == 0) {
    aguardando_primeira_resposta = true;
  }
  get("/system", home_system_cb, show_spinner);
}

void db_home_tick() {
  if (!home_ui_criada) return;
  if (WiFi.status() != WL_CONNECTED || cluster.http == "") return;

  uint32_t agora = millis();
  if (agora - ultimo_tick_check_ms < HOME_CHECK_INTERVAL_MS) return;
  ultimo_tick_check_ms = agora;

  bool home_ativa = false;
  if (bsp_display_lock(30)) {
    home_ativa = (lv_scr_act() == objects.main) &&
                 (lv_tabview_get_tab_act(objects.tv_dashboard) == 0);
    bsp_display_unlock();
  }
  if (!home_ativa) return;

  if (ultimo_request_home_ms > 0 &&
      (agora - ultimo_sucesso_home_ms > HOME_STALE_TIMEOUT_MS) &&
      (!aguardando_primeira_resposta || (agora - ultimo_request_home_ms > 3500))) {
    if (bsp_display_lock(30)) {
      lv_label_set_text(lb_home_status, "Sem resposta");
      lv_obj_set_style_text_color(lb_home_status, lv_palette_main(LV_PALETTE_RED), 0);
      bsp_display_unlock();
    }
  }

  if (!req_atual.pendente && (agora - ultimo_request_home_ms >= HOME_REFRESH_INTERVAL_MS)) {
    lv_info_home(false);
  }
}
