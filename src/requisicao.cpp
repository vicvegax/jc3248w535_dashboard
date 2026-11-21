#include "requisicao.h"
#include "config.h"
#include <esp_bsp.h>
#include "logger.h"

RequisicaoAgendada req_atual = {"", NULL, NULL, false, false};

static void notificar_erro(int httpCode) {
  if (req_atual.error_callback == NULL) return;
  if (bsp_display_lock(500)) {
    req_atual.error_callback(httpCode);
    bsp_display_unlock();
  }
}

<<<<<<< HEAD
void get(String path, HttpCallback callback_processamento) {
  // showHeap();//"ANTES GET");
  exibir_spinner(String("requisicao " + path));
=======
void get(String path, HttpCallback callback_processamento, bool show_spinner, HttpErrorCallback callback_erro) {
  showHeap("ANTES GET");
  if (show_spinner) {
    exibir_spinner();
  }
>>>>>>> 4292549 (melhorias:tema,status,alertas,tempo de tela,modal confirmacao)

  req_atual.url = "http://" + cluster.http + ":" + cluster.port + path;
  req_atual.callback = callback_processamento;
  req_atual.error_callback = callback_erro;
  req_atual.show_spinner = show_spinner;
  req_atual.pendente = true;

  LOG_WARN("REQUISICAO", "GET agendado: %s", req_atual.url.c_str());
}

void requisicoes_pendentes() {
  if (!req_atual.pendente) return;
  req_atual.pendente = false;
  bool usar_spinner = req_atual.show_spinner;

  if (WiFi.status() != WL_CONNECTED) {
<<<<<<< HEAD
    //Serial.println("!!!!!!! Erro: Wi-Fi desconectado.");
    // showHeap();//"DEPOIS GET");
=======
    showHeap("DEPOIS GET");
>>>>>>> 4292549 (melhorias:tema,status,alertas,tempo de tela,modal confirmacao)
    LOG_ERROR("REQUISICAO", "Wifi Desconectado");
    notificar_erro(-100);
    if (usar_spinner) ocultar_spinner();
    return;
  }

  HTTPClient http;
  http.begin(req_atual.url);

  String bearerHeader = "Bearer " + String(cluster.token);
  http.addHeader("Authorization", bearerHeader.c_str());
  http.addHeader("Accept", "application/json");
  http.setTimeout(3500);
  int httpCode = http.GET();
  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, payload);

    if (!error && req_atual.callback != NULL) {
<<<<<<< HEAD
      if (bsp_display_lock(100)) {
=======
      if (bsp_display_lock(500)) {
>>>>>>> 4292549 (melhorias:tema,status,alertas,tempo de tela,modal confirmacao)
        LOG_INFO("REQUISICAO", "Dados recebidos. Processando callback...");
        req_atual.callback(doc);
        bsp_display_unlock();
      } else {
        LOG_ERROR("REQUISICAO", "Nao foi possivel obter o lock do LVGL.");
      }
    } else if (error) {
      LOG_ERROR("REQUISICAO", "Erro no Parse do JSON: %s", error.c_str());
      notificar_erro(-200);
    }
  } else {
    LOG_ERROR("REQUISICAO", "Erro HTTP: %d", httpCode);
    notificar_erro(httpCode);
  }
  http.end();
<<<<<<< HEAD
  // showHeap();//"DEPOIS GET");
  ocultar_spinner();

=======
  showHeap("DEPOIS GET");
  if (usar_spinner) ocultar_spinner();
>>>>>>> 4292549 (melhorias:tema,status,alertas,tempo de tela,modal confirmacao)
}
