#ifndef REQUISICAO_H
#define REQUISICAO_H

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// Define um "tipo" para a nossa função de callback que processará o JSON
typedef void (*HttpCallback)(JsonDocument& doc);

// Estrutura para agendar a requisição fora do loop do LVGL
struct RequisicaoAgendada {
    String url;
    HttpCallback callback;
    bool pendente;
};

// Variável global para agendamento (será gerenciada no loop)
extern RequisicaoAgendada req_atual;

// Função genérica para disparar o GET (chame esta função nas suas Actions)
// Adicionar a "/" no inicio
void get(String path, HttpCallback callback_processamento);

// Função que roda no loop principal para processar sem travar a tela
void requisicoes_pendentes();

#endif
