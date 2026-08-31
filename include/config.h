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


bool iniciar_wifi_salvo();
bool salvar_e_conectar(String novo_ssid, String nova_senha);
void esquecer_wifi();
void exibir_spinner();
void ocultar_spinner();

void showHeap();