#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>

struct Cluster {
  int id;
  String nome;
  String http;
  String token;
  int port;
};

extern Cluster cluster;

extern const int MAX_CLUSTERS;

extern bool loopListaWifi;
// Declaração das funções (implementadas abaixo)
void salvarClusters(Cluster itens[], int total);
int carregarClusters(Cluster itens[]);
void adicionarCluster(Cluster novoItem);
void editarCluster(int index, Cluster itemEditado);
// void listarClusters();
Cluster pegarCluster(int index);
void deletarCluster(int index);

void atualizaClusterGlobal(int id);
int obterClusterAtivoIndex();


bool iniciar_wifi_salvo();
bool salvar_e_conectar(String novo_ssid, String nova_senha);
void esquecer_wifi();
void exibir_spinner(String onde);
void ocultar_spinner();
void exibir_confirmacao(const char *titulo, const char *mensagem, const char *txt_confirmar, void (*on_confirm)());

int obterBrilhoTela();
void salvarBrilhoTela(int percent);
int obterTimeoutTelaSeg();
void salvarTimeoutTelaSeg(int segundos);

void bateria_init();
float bateria_ler_tensao();
int bateria_percentual(float v);

void showHeap();