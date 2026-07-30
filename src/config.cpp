#include <WiFi.h>
#include <Preferences.h>
#include "config.h"
#include "ui/ui.h"

Preferences prefs;
String url = "";
String token = "";

// Tenta conectar com o que está salvo
bool iniciar_wifi_salvo() {
    prefs.begin("config", true); // true = Somente leitura
    String ssid = prefs.getString("ssid", "");
    String pass = prefs.getString("pass", "");
    prefs.end();

    if(ssid == "") return false;

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
        Serial.println("Conectado com sucesso!");
        url = "http://" + ler_http() + ":" + ler_port();
        token = ler_token();

        return true;
    } else {
        WiFi.disconnect();      // Cancela violentamente qualquer tentativa de conexão travada
        delay(200);             // Dá um respiro de 100ms para o hardware limpar o estado
        Serial.println("Falha ao conectar.");
        return false;
    }
}

// Salva e conecta (chamado pelo botão da aba Config)
bool salvar_e_conectar(String novo_ssid, String nova_senha) {
    prefs.begin("config", false); // false = Leitura e Escrita
    prefs.putString("ssid", novo_ssid);
    prefs.putString("pass", nova_senha);
    prefs.end();

    return iniciar_wifi_salvo();
}

void salvar_http_port(String novo_http, int novo_port) {
    prefs.begin("config", false); // false = Leitura e Escrita
    prefs.putString("http", novo_http);
    prefs.putInt("port", novo_port);
    prefs.end();
}

String ler_token() {
    prefs.begin("config", true); // true = Somente leitura
    String http = prefs.getString("token", "abcd1234");
    prefs.end();
    return http;
}

String ler_http() {
    prefs.begin("config", true); // true = Somente leitura
    String http = prefs.getString("http", "");
    prefs.end();
    return http;
}

int ler_port() {
    prefs.begin("config", true); // true = Somente leitura
    int port = prefs.getInt("port", 0);
    prefs.end();
    return port;
}

void esquecer_wifi() {
    prefs.begin("config", false);
    prefs.putString("ssid", "");
    prefs.putString("pass", "");
    prefs.end();
}

void exibir_spinner() {
  lv_obj_clear_flag(objects.pn_spinner, LV_OBJ_FLAG_HIDDEN);
  lv_refr_now(NULL);
}

void ocultar_spinner() {
  lv_obj_add_flag(objects.pn_spinner, LV_OBJ_FLAG_HIDDEN);
  lv_refr_now(NULL);
}