#include "ui/ui.h"
#include "db_docker.h"
#include "requisicao.h"
#include <cstring>
#include <logger.h>

extern lv_style_t estilo_checked;

static void lv_info_container_cb(JsonDocument& doc);
static void lv_bt_cont_cb(lv_event_t *e);
static void lv_lista_dockers_cb(JsonDocument& doc);

void lv_lista_containers() {
  //Limpa primeiro
  uint32_t itens_atuais = lv_obj_get_child_cnt(objects.ls_cont);
  for (uint32_t i = 0; i < itens_atuais; i++) {
    lv_obj_t * item = lv_obj_get_child(objects.ls_cont, i);
    void * user_data = lv_obj_get_user_data(item);
    if (user_data != NULL) {
      free(user_data);
      lv_obj_set_user_data(item, NULL);
    }
  }
  lv_obj_clean(objects.ls_cont); 

  //Faz requisição
  get("/containers", lv_lista_dockers_cb);
}

static void lv_lista_dockers_cb(JsonDocument& doc) {
  // 1. Limpa os itens antigos de ambas as listas para evitar duplicação
  // Certifique-se de usar os nomes exatos gerados pelo EEZ Studio

  char buffer_texto[64]; // Buffer para armazenar a string formatada "nome - id"
  // 2. PROCESSA OS CONTAINERS ATIVOS
  lv_list_add_text(objects.ls_cont, "Ativos");
  JsonArray ativos = doc["ativos"].as<JsonArray>();
  for(JsonVariant container : ativos) {
    const char* name = container["name"].as<const char*>();
    const char* id   = container["id"].as<const char*>();
    
    // Garante que os dados existem antes de formatar
    if (name && id) {
      // Formata a string no formato "nome - id" de forma segura
      snprintf(buffer_texto, sizeof(buffer_texto), "%s", name);
      
      // Adiciona o item no List de ativos. O segundo parâmetro pode ser um ícone (ex: LV_SYMBOL_PLAY)
      // lv_list_add_btn(objects.ls_cont_ativos, LV_SYMBOL_PLAY, buffer_texto);
      size_t id_tamanho = strlen(id) + 1;
      char * id_armazenado = (char *)malloc(id_tamanho);
      if (id_armazenado == NULL) {
        LOG_ERROR("DOCKER", "Erro: memoria insuficiente para armazenar ID do container.");
        continue;
      }
      memcpy(id_armazenado, id, id_tamanho);

      lv_obj_t * btn = lv_list_add_btn(objects.ls_cont, NULL, buffer_texto);
      lv_obj_set_user_data(btn, (void *)id_armazenado);
      lv_obj_add_flag(btn, LV_OBJ_FLAG_CHECKABLE);
      lv_obj_add_event_cb(btn, lv_bt_cont_cb, LV_EVENT_CLICKED, NULL);
      lv_obj_add_style(btn, &estilo_checked, LV_STATE_CHECKED);
    }
  }

  // 3. PROCESSA OS CONTAINERS INATIVOS
  lv_list_add_text(objects.ls_cont, "Inativos");
  JsonArray inativos = doc["inativos"].as<JsonArray>();
  for(JsonVariant container : inativos) {
    const char* name = container["name"].as<const char*>();
    const char* id   = container["id"].as<const char*>();
    
    if (name && id) {
      // Formata a string no formato "nome - id"
      snprintf(buffer_texto, sizeof(buffer_texto), "%s - %s", id, name);
      
      // Adiciona o item no List de inativos (ex: usando o ícone de STOP)
      // lv_list_add_btn(objects.ls_cont_inativos, LV_SYMBOL_STOP, buffer_texto);
      lv_list_add_btn(objects.ls_cont, NULL, buffer_texto);
    }
  }
}

static void lv_bt_cont_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj = lv_event_get_target(e);
    if(code == LV_EVENT_CLICKED) {
      uint32_t total_itens = lv_obj_get_child_cnt(objects.ls_cont);
      for(uint32_t i = 0; i < total_itens; i++) {
          lv_obj_t * filho = lv_obj_get_child(objects.ls_cont, i);
          
          // Se o filho for um botão e NÃO for o que acabou de ser clicado, desmarca ele
          if(filho != obj) {
              lv_obj_clear_state(filho, LV_STATE_CHECKED);
          }
      }
      
      // 3. Força o botão clicado a ficar permanentemente no estado MARCADO/DESTACADO
      lv_obj_add_state(obj, LV_STATE_CHECKED);

      char * container_id = (char *)lv_obj_get_user_data(obj);
      if (container_id != NULL) {
          // PRONTO! Dados isolados perfeitamente com zero processamento de string
          get("/container/" + String(container_id), lv_info_container_cb);
          // Serial.printf("\n[DOCKER] Clique detectado de forma nativa:\n");
          // Serial.printf("ID do Container:   %s\n", container_id);
          
          // Aqui você já pode chamar sua API para dar Start/Stop usando o ID puro
          // ex: enviar_comando_api(container_id, "stop");
      }
      
    }
}

static void lv_info_container_cb(JsonDocument& doc) {
  String id_container = doc["containerId"].as<String>();

  // VARIÁVEIS DE CPU (Acessando o sub-objeto "cpu")
  String cpu_percent = doc["cpu"]["percent"].as<String>();
  int cpu_online     = doc["cpu"]["onlineCpus"].as<int>();

  // VARIÁVEIS DE MEMÓRIA RAM (Acessando o sub-objeto "ram")
  String ram_usage   = doc["ram"]["usageMB"].as<String>();
  String ram_limit   = doc["ram"]["limitMB"].as<String>();
  String ram_percent = doc["ram"]["percent"].as<String>();
  lv_label_set_text_fmt(objects.lb_cont_cpu, "CPU\n%s", cpu_percent.c_str());
  lv_label_set_text_fmt(objects.lb_cont_ram, "RAM\n%s", ram_usage.c_str());

  // Serial.println("--- MÉTRICAS DO CONTAINER ---");
  // Serial.printf("ID: %s\n", id_container.c_str());
  // Serial.printf("CPU Uso: %s (CPUs Disponiveis: %d)\n", cpu_percent.c_str(), cpu_online);
  // Serial.printf("RAM Uso: %s de %s (%s)\n", ram_usage.c_str(), ram_limit.c_str(), ram_percent.c_str());
  // Serial.println("-----------------------------");
}