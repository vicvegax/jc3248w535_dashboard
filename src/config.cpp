#include <WiFi.h>
#include <Preferences.h>
#include <time.h>
#include "config.h"
#include "ui/ui.h"
#include <esp_bsp.h>
#include "logger.h"

Preferences prefs;
Cluster cluster;
bool loopListaWifi = false;

// extern SemaphoreHandle_t lvgl_mutex; 

const char* CLUSTER_NAMESPACE = "clusters";
const char* CLUSTER_KEY = "json";
const char* CLUSTER_ACTIVE_KEY = "ativo_idx";
const int MAX_CLUSTERS = 20;

void showHeap() {
  // Serial.printf("**** %s: Heap Livre: %d bytes\n", msg, ESP.getFreeHeap());
  // Serial.printf("**** %s: Maior bloco livre: %d bytes\n", msg, ESP.getMinFreeHeap()); // Menor nível que o heap já chegou
  if(ESP.getMinFreeHeap() < 100000) {
    LOG_WARN_1("HEAP", "Abaixo de 100k: %d", ESP.getMinFreeHeap());
  }
}

// Tenta conectar com o que está salvo
bool iniciar_wifi_salvo() {
  prefs.begin("wifi", true); // true = Somente leitura
  String ssid = prefs.getString("ssid", "");
  String pass = prefs.getString("pass", "");
  prefs.end();

  if(ssid == "") return false;

  LOG_INFO("WIFI", "Tentando conectar com Wi-Fi salvo: %s", ssid);
  // --- O SEGREDO ESTÁ NESTAS 3 LINHAS ---
  WiFi.mode(WIFI_STA);    // Garante que o chip está operando como Estação (Cliente)
  WiFi.disconnect();      // Cancela violentamente qualquer tentativa de conexão travada
  delay(200);             // Dá um respiro de 100ms para o hardware limpar o estado

  WiFi.begin(ssid.c_str(), pass.c_str());
  int tentativas = 0;
  while (WiFi.status() != WL_CONNECTED && tentativas < 10) {
    delay(500);
    tentativas++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    LOG_INFO("WIFI", "Conectado com sucesso! Carregando cluster salvo...");
    configTime(-3 * 3600, 0, "a.st1.ntp.br", "pool.ntp.org");
    atualizaClusterGlobal(obterClusterAtivoIndex());
    return true;
  } else {
    WiFi.disconnect();      // Cancela violentamente qualquer tentativa de conexão travada
    delay(200);             // Dá um respiro de 100ms para o hardware limpar o estado
    LOG_ERROR("WIFI", "Falha ao conectar!");
    return false;
  }
}

int obterClusterAtivoIndex() {
  prefs.begin(CLUSTER_NAMESPACE, true);
  int idx = prefs.getInt(CLUSTER_ACTIVE_KEY, 0);
  prefs.end();
  return idx;
}

void atualizaClusterGlobal(int id) {
  Cluster itens[MAX_CLUSTERS];
  int total = carregarClusters(itens);

  if (total <= 0) {
    cluster = {"", "", "", 0};
    id = -1;
  } else {
    if (id < 0 || id >= total) {
      id = 0;
    }
    cluster = itens[id];
  }

  prefs.begin(CLUSTER_NAMESPACE, false);
  prefs.putInt(CLUSTER_ACTIVE_KEY, id);
  prefs.end();

  String msg = "Cluster nao selecionado!";
  if (cluster.http != "") {
    msg = "Cluster: " + cluster.http + ':' + cluster.port;
  }

<<<<<<< HEAD
  // if (xSemaphoreTake(lvgl_mutex, portMAX_DELAY) == pdTRUE) {
  if(bsp_display_lock(100)) {
    lv_label_set_text(objects.lb_cluster, msg.c_str());
    bsp_display_unlock();
} else {
  LOG_ERROR("CLUSTER", "Erro liberando display.");
}
    // xSemaphoreGive(lvgl_mutex); // Libera o LVGL para desenhar os novos itens
  // }
=======
  bsp_display_lock(1000);
  lv_label_set_text(objects.lb_cluster, msg.c_str());
  bsp_display_unlock();
>>>>>>> 4292549 (melhorias:tema,status,alertas,tempo de tela,modal confirmacao)
}

// Salva e conecta (chamado pelo botão da aba Config)
bool salvar_e_conectar(String novo_ssid, String nova_senha) {
  Serial.printf("Salvando SSID: %s e Senha: %s\n", novo_ssid.c_str(), nova_senha.c_str());
  prefs.begin("wifi", false); // false = Leitura e Escrita
  prefs.putString("ssid", novo_ssid);
  prefs.putString("pass", nova_senha);
  prefs.end();

  return iniciar_wifi_salvo();
}

void esquecer_wifi() {
  Serial.println("Esquecendo Wi-Fi salvo...");
  WiFi.disconnect(true, true); // Desconecta e apaga credenciais
  delay(200);
  prefs.begin("wifi", false);
  prefs.putString("ssid", "");
  prefs.putString("pass", "");
  prefs.end();
}

<<<<<<< HEAD
void exibir_spinner(String onde) {
  LOG_SPINNER(onde.c_str(), "MOSTRANDO SPINNER");
=======
int obterBrilhoTela() {
  prefs.begin("display", true);
  int brilho = prefs.getInt("brilho", 25);
  prefs.end();
  if (brilho < 10) brilho = 10;
  if (brilho > 100) brilho = 100;
  return brilho;
}

void salvarBrilhoTela(int percent) {
  if (percent < 10) percent = 10;
  if (percent > 100) percent = 100;
  prefs.begin("display", false);
  prefs.putInt("brilho", percent);
  prefs.end();
}

int obterTimeoutTelaSeg() {
  prefs.begin("display", true);
  int seg = prefs.getInt("timeout_s", 30);
  prefs.end();
  return seg;
}

void salvarTimeoutTelaSeg(int segundos) {
  prefs.begin("display", false);
  prefs.putInt("timeout_s", segundos);
  prefs.end();
}

static constexpr int BAT_ADC_PIN = 5;
static constexpr float BAT_DIVIDER_RATIO = 2.0f;
static float bat_tensao_suavizada = 0.0f;

void bateria_init() {
  analogReadResolution(12);
  analogSetPinAttenuation(BAT_ADC_PIN, ADC_11db);
  bat_tensao_suavizada = 0.0f;
}

float bateria_ler_tensao() {
  uint32_t soma_mv = 0;
  constexpr int AMOSTRAS = 16;
  for (int i = 0; i < AMOSTRAS; i++) {
    soma_mv += analogReadMilliVolts(BAT_ADC_PIN);
  }
  float v_atual = ((soma_mv / static_cast<float>(AMOSTRAS)) / 1000.0f) * BAT_DIVIDER_RATIO;
  if (bat_tensao_suavizada <= 0.1f) {
    bat_tensao_suavizada = v_atual;
  } else {
    bat_tensao_suavizada = (bat_tensao_suavizada * 0.7f) + (v_atual * 0.3f);
  }
  return bat_tensao_suavizada;
}

int bateria_percentual(float v) {
  // Curva aproximada Li-Ion/LiPo (3.20V = 0%, 4.20V = 100%)
  if (v <= 3.20f) return 0;
  if (v >= 4.20f) return 100;
  if (v >= 4.00f) return 80 + static_cast<int>(((v - 4.00f) / 0.20f) * 20.0f);
  if (v >= 3.75f) return 40 + static_cast<int>(((v - 3.75f) / 0.25f) * 40.0f);
  if (v >= 3.55f) return 15 + static_cast<int>(((v - 3.55f) / 0.20f) * 25.0f);
  return static_cast<int>(((v - 3.20f) / 0.35f) * 15.0f);
}

void exibir_spinner() {
  // Serial.println("Exibindo spinner...");
  // bsp_display_lock(1000);
>>>>>>> 4292549 (melhorias:tema,status,alertas,tempo de tela,modal confirmacao)
  lv_obj_clear_flag(objects.pn_spinner, LV_OBJ_FLAG_HIDDEN);
}

void ocultar_spinner() {
<<<<<<< HEAD
  if(bsp_display_lock(150)) {
    lv_obj_add_flag(objects.pn_spinner, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(objects.pn_spinner, LV_OBJ_FLAG_USER_1);
    lv_refr_now(NULL);
=======
  if (bsp_display_lock(200)) {
    lv_obj_add_flag(objects.pn_spinner, LV_OBJ_FLAG_HIDDEN);
>>>>>>> 4292549 (melhorias:tema,status,alertas,tempo de tela,modal confirmacao)
    bsp_display_unlock();
  } else {
    lv_obj_add_flag(objects.pn_spinner, LV_OBJ_FLAG_USER_1);
    LOG_ERROR("SPINNER", "Erro ao ocultar");
  }
<<<<<<< HEAD
=======
}

namespace {
lv_obj_t *modal_confirm_bg = nullptr;
void (*modal_confirm_cb)() = nullptr;

void fechar_modal_confirm() {
  if (modal_confirm_bg) {
    lv_obj_del(modal_confirm_bg);
    modal_confirm_bg = nullptr;
  }
  modal_confirm_cb = nullptr;
}

void modal_cancelar_cb(lv_event_t *e) {
  if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
    fechar_modal_confirm();
  }
}

void modal_ok_cb(lv_event_t *e) {
  if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
    void (*cb)() = modal_confirm_cb;
    fechar_modal_confirm();
    if (cb) cb();
  }
}
} // namespace

void exibir_confirmacao(const char *titulo, const char *mensagem, const char *txt_confirmar, void (*on_confirm)()) {
  fechar_modal_confirm();
  modal_confirm_cb = on_confirm;

  modal_confirm_bg = lv_obj_create(lv_layer_top());
  lv_obj_set_size(modal_confirm_bg, lv_pct(100), lv_pct(100));
  lv_obj_set_pos(modal_confirm_bg, 0, 0);
  lv_obj_clear_flag(modal_confirm_bg, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_bg_color(modal_confirm_bg, lv_color_black(), 0);
  lv_obj_set_style_bg_opa(modal_confirm_bg, 170, 0);
  lv_obj_set_style_border_width(modal_confirm_bg, 0, 0);

  lv_obj_t *card = lv_obj_create(modal_confirm_bg);
  lv_obj_set_size(card, 320, 156);
  lv_obj_center(card);
  lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_bg_color(card, lv_color_hex(0x181B22), 0);
  lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
  lv_obj_set_style_border_color(card, lv_color_hex(0x323846), 0);
  lv_obj_set_style_border_width(card, 1, 0);
  lv_obj_set_style_radius(card, 10, 0);
  lv_obj_set_style_pad_all(card, 14, 0);

  lv_obj_t *lb_tit = lv_label_create(card);
  lv_label_set_text(lb_tit, titulo);
  lv_obj_set_style_text_font(lb_tit, &lv_font_montserrat_16, 0);
  lv_obj_set_style_text_color(lb_tit, lv_color_white(), 0);
  lv_obj_set_pos(lb_tit, 0, 0);

  lv_obj_t *lb_msg = lv_label_create(card);
  lv_label_set_text(lb_msg, mensagem);
  lv_label_set_long_mode(lb_msg, LV_LABEL_LONG_WRAP);
  lv_obj_set_width(lb_msg, 290);
  lv_obj_set_style_text_font(lb_msg, &lv_font_montserrat_12, 0);
  lv_obj_set_style_text_color(lb_msg, lv_color_hex(0xD0D7DE), 0);
  lv_obj_set_pos(lb_msg, 0, 32);

  lv_obj_t *bt_canc = lv_btn_create(card);
  lv_obj_set_size(bt_canc, 134, 38);
  lv_obj_align(bt_canc, LV_ALIGN_BOTTOM_LEFT, 0, 0);
  lv_obj_set_style_bg_color(bt_canc, lv_color_hex(0x262B36), 0);
  lv_obj_add_event_cb(bt_canc, modal_cancelar_cb, LV_EVENT_CLICKED, NULL);
  lv_obj_t *lb_canc = lv_label_create(bt_canc);
  lv_label_set_text(lb_canc, "Cancelar");
  lv_obj_center(lb_canc);

  lv_obj_t *bt_ok = lv_btn_create(card);
  lv_obj_set_size(bt_ok, 134, 38);
  lv_obj_align(bt_ok, LV_ALIGN_BOTTOM_RIGHT, 0, 0);
  lv_obj_set_style_bg_color(bt_ok, lv_palette_main(LV_PALETTE_RED), 0);
  lv_obj_add_event_cb(bt_ok, modal_ok_cb, LV_EVENT_CLICKED, NULL);
  lv_obj_t *lb_ok = lv_label_create(bt_ok);
  lv_label_set_text(lb_ok, txt_confirmar);
  lv_obj_center(lb_ok);
>>>>>>> 4292549 (melhorias:tema,status,alertas,tempo de tela,modal confirmacao)
}



// Grava todo o array atualizado na memória flash
void salvarClusters(Cluster itens[], int total) {
  Serial.printf("Salvando %d clusters na memória flash...\n", total);
  JsonDocument doc; // Aloca memória para o JSON
  JsonArray arr = doc.to<JsonArray>();

  for (int i = 0; i < total; i++) {
    JsonObject obj = arr.add<JsonObject>();
    obj["nome"] = itens[i].nome;
    obj["http"] = itens[i].http;
    obj["port"] = itens[i].port;
    obj["token"] = itens[i].token;
  }

  String jsonString;
  serializeJson(doc, jsonString);

  prefs.begin(CLUSTER_NAMESPACE, false);
  prefs.putString(CLUSTER_KEY, jsonString);
  prefs.end();
}

// Recupera o JSON da flash e preenche o array. Retorna a quantidade de itens.
int carregarClusters(Cluster itens[]) {
  prefs.begin(CLUSTER_NAMESPACE, true); // Modo leitura
  String jsonString = prefs.getString(CLUSTER_KEY, "[]");
  prefs.end();

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, jsonString);
  
  if (error) {
    LOG_ERROR_1("CLUSTER", "Não há cluster salvo");
    return 0; // Retorna vazio se falhar ou não existir
  }

  JsonArray arr = doc.as<JsonArray>();
  int total = arr.size();
  if (total > MAX_CLUSTERS) total = MAX_CLUSTERS;

  for (int i = 0; i < total; i++) {
    itens[i].id = i;
    itens[i].nome = arr[i]["nome"].as<String>();
    itens[i].http = arr[i]["http"].as<String>();
    itens[i].port = arr[i]["port"].as<int>();
    itens[i].token = arr[i]["token"].as<String>();
  }

  LOG_INFO_1("CLUSTER", "Carregados %d clusters.", total);
  return total;
}

void adicionarCluster(Cluster novoItem) {
  Cluster itens[MAX_CLUSTERS];
  int total = carregarClusters(itens);

  if (total >= MAX_CLUSTERS) {
    Serial.println("Erro: Limite máximo de 20 itens atingido!");
    return;
  }

  novoItem.id = total;
  itens[total] = novoItem; // Insere no final
  salvarClusters(itens, total + 1);
  Serial.println("Cluster adicionado com sucesso!");
}

void editarCluster(int index, Cluster itemEditado) {
  Cluster itens[MAX_CLUSTERS];
  int total = carregarClusters(itens);

  if (index < 0 || index >= total) {
    Serial.println("Erro: Índice inválido para edição!");
    return;
  }

  itens[index] = itemEditado; // Substitui o item no índice
  salvarClusters(itens, total);
  Serial.println("Cluster editado com sucesso!");
}

Cluster pegarCluster(int index) {
  Cluster itens[MAX_CLUSTERS];
  int total = carregarClusters(itens);

  if (index < 0 || index >= total) {
    // Serial.printf("Erro: Índice inválido! %d itens disponíveis.\n", total);
    LOG_ERROR_1("CLUSTER", "Erro: Índice inválido! %d itens disponíveis.", total);
    return {-1, "", "", "", 0}; // Retorna um item vazio em caso de erro
  }

  LOG_INFO_1("CLUSTER", "Usando [%d]: %s - %s:%d", index, itens[index].nome.c_str(), itens[index].http.c_str(), itens[index].port);
  // Serial.printf("Pegando cluster no índice [%d]: %s - %s:%d\n", index, itens[index].nome.c_str(), itens[index].http.c_str(), itens[index].port);
  return itens[index];
}

void deletarCluster(int index) {
  Cluster itens[MAX_CLUSTERS];
  int total = carregarClusters(itens);

  // Valida se o índice existe no array atual
  if (index < 0 || index >= total) {
    Serial.println("Erro: Índice inválido para exclusão!");
    return;
  }

  // Desloca todos os itens após o índice deletado uma posição para trás
  for (int i = index; i < total - 1; i++) {
    itens[i] = itens[i + 1];
  }

  // Salva o array com uma unidade a menos no total
  salvarClusters(itens, total - 1);
  Serial.printf("Cluster no índice [%d] removido com sucesso!\n", index);
}
