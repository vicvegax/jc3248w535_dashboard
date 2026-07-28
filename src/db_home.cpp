#include <Arduino.h>
#include "ui/ui.h"

void db_home_conectado() {
    // Altera um label que você colocou na tela Home no EEZ Studio
    lv_label_set_text(objects.lb_conectado, "Conectado!");
}

void db_home_desconectado() {
    // Altera um label que você colocou na tela Home no EEZ Studio
    lv_label_set_text(objects.lb_conectado, "* DESCONECTADO *");
}

// Quando o WiFi conectar e o MQTT receber dados, você chama funções aqui
void tab_home_atualizar_dado_mqtt(const char* topico, const char* valor) {
    // Tratar payload do MQTT e jogar na tela...
}