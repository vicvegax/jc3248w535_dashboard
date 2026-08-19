#include "requisicao.h"
#include "config.h"
#include <esp_bsp.h>

// extern SemaphoreHandle_t lvgl_mutex; 

RequisicaoAgendada req_atual = {"", NULL, false};

void get(String path, HttpCallback callback_processamento) {
  showHeap("ANTES GET");
  exibir_spinner();

  // Apenas agenda a requisição para o loop principal executar fora do LVGL
  req_atual.url = "http://" + cluster.http + ":" + cluster.port + path;
  req_atual.callback = callback_processamento;
  req_atual.pendente = true;

  Serial.printf("Requisição GET agendada: %s\n", req_atual.url.c_str());
}

void requisicoes_pendentes() {
  if (!req_atual.pendente) return;
  req_atual.pendente = false; // Consome o agendamento

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("!!!!!!! Erro: Wi-Fi desconectado.");
    ocultar_spinner();
    showHeap("DEPOIS GET");
    return;
  }
  
  HTTPClient http;
  http.begin(req_atual.url);
  
  String bearerHeader = "Bearer " + String(cluster.token);
  http.addHeader("Authorization", bearerHeader.c_str());
  // (Opcional) Garante que o servidor saiba que você espera um JSON
  http.addHeader("Accept", "application/json"); 
  http.setTimeout(2000);
  int httpCode = http.GET();
  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
   
    // Aloca o buffer do JSON de forma dinâmica
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, payload);
    
    // Se o JSON for válido e houver uma função cadastrada, envia os dados
    if (!error && req_atual.callback != NULL) {
      // if (xSemaphoreTake(lvgl_mutex, portMAX_DELAY) == pdTRUE) {
        Serial.println("Processando callback da requisição...");
        req_atual.callback(doc);
        // xSemaphoreGive(lvgl_mutex); // Libera o LVGL para desenhar os novos itens
      // }

    } else if (error) {
    Serial.printf("Erro no Parse do JSON: %s\n", error.c_str());
    }
  } else {
    Serial.printf("Erro HTTP: %d\n", httpCode);
  }
  http.end();
  ocultar_spinner();

  showHeap("DEPOIS GET");
}
