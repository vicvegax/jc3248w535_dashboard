#include "requisicao.h"
#include "config.h"
#include <esp_bsp.h>
#include "logger.h"

// extern SemaphoreHandle_t lvgl_mutex; 

RequisicaoAgendada req_atual = {"", NULL, false};

void get(String path, HttpCallback callback_processamento) {
  // showHeap();//"ANTES GET");
  exibir_spinner();

  // Apenas agenda a requisição para o loop principal executar fora do LVGL
  req_atual.url = "http://" + cluster.http + ":" + cluster.port + path;
  req_atual.callback = callback_processamento;
  req_atual.pendente = true;

  LOG_WARN("REQUISICAO", "GET agendado: %s", req_atual.url.c_str());
}

void requisicoes_pendentes() {
  if (!req_atual.pendente) return;
  req_atual.pendente = false; // Consome o agendamento

  if (WiFi.status() != WL_CONNECTED) {
    //Serial.println("!!!!!!! Erro: Wi-Fi desconectado.");
    // showHeap();//"DEPOIS GET");
    LOG_ERROR("REQUISICAO", "Wifi Desconectado");
    ocultar_spinner();
    return;
  }
  // LOG_WARN_1("REQUISICAO", "Processando...");
  HTTPClient http;
  http.begin(req_atual.url);
  
  String bearerHeader = "Bearer " + String(cluster.token);
  http.addHeader("Authorization", bearerHeader.c_str());
  // (Opcional) Garante que o servidor saiba que você espera um JSON
  http.addHeader("Accept", "application/json"); 
  http.setTimeout(2000);
  int httpCode = http.GET();
  if (httpCode == HTTP_CODE_OK) {
    // LOG_INFO_1("REQUISICAO", "Dados foram recebidos...");
    String payload = http.getString();
   
    // Aloca o buffer do JSON de forma dinâmica
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, payload);
    
    // Se o JSON for válido e houver uma função cadastrada, envia os dados
    if (!error && req_atual.callback != NULL) {
      if (bsp_display_lock(100)) {
        LOG_INFO("REQUISICAO", "Dados recebidos. Processando callback...");
        req_atual.callback(doc);
        bsp_display_unlock();
      } else {
        LOG_ERROR("REQUISICAO", "Nao foi possivel obter o lock do LVGL.");
      }

    } else if (error) {
      LOG_ERROR("REQUISICAO", "Erro no Parse do JSON: %s", error.c_str());
    }
  } else {
    LOG_ERROR("REQUISICAO", "Erro HTTP: %d", httpCode);
  }
  http.end();
  // showHeap();//"DEPOIS GET");
  ocultar_spinner();

}
