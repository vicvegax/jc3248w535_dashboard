#include "ui/ui.h"
#include "db_rede.h"
#include "requisicao.h"
#include <esp_bsp.h>
#include <logger.h>

static void lv_info_rede_cb(JsonDocument& doc);
static String json_to_string(const JsonVariant& value);

void lv_info_rede() {
  lv_label_set_text(objects.lb_download, "Download: ...");
  lv_label_set_text(objects.lb_upload, "Upload: ...");
  lv_label_set_text(objects.lb_ping, "Ping: ...");

  get("/network", lv_info_rede_cb);
}

static String json_to_string(const JsonVariant& value) {
  // Serial.println(value.as<String>());
  if (value.isNull()) {
    return String("(NULO)");
  }

  return String(value.as<String>());
}

static void lv_info_rede_cb(JsonDocument& doc) {
  // 1. Limpa os itens antigos de ambas as listas para evitar duplicação
  // Certifique-se de usar os nomes exatos gerados pelo EEZ Studio

  const String download = "Download: " + json_to_string(doc["download"]);
  const String upload = "Upload: " + json_to_string(doc["upload"]);
  const String ping = "Ping: " + json_to_string(doc["ping_ms"]) + " ms";

  lv_label_set_text(objects.lb_download, download.c_str());
  lv_label_set_text(objects.lb_upload, upload.c_str());
  lv_label_set_text(objects.lb_ping, ping.c_str());

}