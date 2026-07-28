#pragma once
#include <Arduino.h>

bool iniciar_wifi_salvo();
bool salvar_e_conectar(String novo_ssid, String nova_senha);
void esquecer_wifi();
void salvar_http_port(String novo_http, int novo_port);
String ler_http();
int ler_port();
