#include <Arduino.h>
#include "ui/ui.h"
#include "WiFi.h"

void db_conectado() {
  String meu_ip = "Conectado como " + WiFi.localIP().toString();
  lv_label_set_text(objects.lb_conectado, meu_ip.c_str());
}

void db_desconectado() {
  // Altera um label que você colocou na tela Home no EEZ Studio
  lv_label_set_text(objects.lb_conectado, "* DESCONECTADO *");
}
