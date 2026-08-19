#include <WiFi.h>
#include <Preferences.h>
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
const int MAX_CLUSTERS = 20;

void showHeap(String msg) {
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
    LOG_INFO("WIFI", "Conectado com sucesso! Carregando 1º cluster salvo...");
    atualizaClusterGlobal(0);
    return true;
  } else {
    WiFi.disconnect();      // Cancela violentamente qualquer tentativa de conexão travada
    delay(200);             // Dá um respiro de 100ms para o hardware limpar o estado
    LOG_ERROR("WIFI", "Falha ao conectar!");
    return false;
  }
}

void atualizaClusterGlobal(int id) {
  cluster = pegarCluster(id);
  String msg = "Cluster nao selecionado!";
  if (cluster.http != "") {
    msg = "Cluster: " + cluster.http + ':' + cluster.port;
    // LOG_INFO("CLUSTER", "Usando %s", msg.c_str());
  // } else {
    // LOG_ERROR("CLUSTER", "Não selecionado.");
  }

  // if (xSemaphoreTake(lvgl_mutex, portMAX_DELAY) == pdTRUE) {
  bsp_display_lock(1000);
  lv_label_set_text(objects.lb_cluster, msg.c_str());
  bsp_display_unlock();
    // xSemaphoreGive(lvgl_mutex); // Libera o LVGL para desenhar os novos itens
  // }
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
  delay(200); // Dá um respiro de 200ms para o hardware limpar o estado
  prefs.begin("config", false);
  prefs.putString("ssid", "");
  prefs.putString("pass", "");
  prefs.end();
}

void exibir_spinner() {
  // Serial.println("Exibindo spinner...");
  // bsp_display_lock(1000);
  lv_obj_clear_flag(objects.pn_spinner, LV_OBJ_FLAG_HIDDEN);
  // lv_refr_now(NULL);
  // bsp_display_unlock();
}

void ocultar_spinner() {
  if(bsp_display_lock(0)) {
    lv_obj_add_flag(objects.pn_spinner, LV_OBJ_FLAG_HIDDEN);
    lv_refr_now(NULL);
    bsp_display_unlock();
  } else {
    LOG_ERROR("SPINNER", "Erro ao ocultar");
  }
  // Serial.println("Ocultando spinner...");
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
    return {"", "", "", 0}; // Retorna um item vazio em caso de erro
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
