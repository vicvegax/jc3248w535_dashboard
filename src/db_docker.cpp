#include "ui/ui.h"
#include "db_docker.h"
#include "requisicao.h"

static void cont_stat_cb(JsonDocument& doc);
static void bt_cont_cb(lv_event_t *e);

void list_dockers_cb(JsonDocument& doc) {
  // 1. Limpa os itens antigos de ambas as listas para evitar duplicação
  // Certifique-se de usar os nomes exatos gerados pelo EEZ Studio
  lv_obj_clean(objects.ls_cont); 

  char buffer_texto[64]; // Buffer para armazenar a string formatada "nome - id"
  lv_obj_t * btn;
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
      char * id_armazenado = (char *)malloc(13);
      strcpy(id_armazenado, id);

      btn = lv_list_add_btn(objects.ls_cont, NULL, buffer_texto);
      lv_obj_set_user_data(btn, (void *)id_armazenado);
      lv_obj_add_flag(btn, LV_OBJ_FLAG_CHECKABLE);
      lv_obj_add_event_cb(btn, bt_cont_cb, LV_EVENT_CLICKED, NULL);
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

static void bt_cont_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj = lv_event_get_target(e);
    if(code == LV_EVENT_CLICKED) {
      lv_obj_t * lista_pai = lv_obj_get_parent(objects.ls_cont);
      uint32_t total_itens = lv_obj_get_child_cnt(lista_pai);
      for(uint32_t i = 0; i < total_itens; i++) {
          lv_obj_t * filho = lv_obj_get_child(lista_pai, i);
          
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
          get("/container/" + String(container_id), cont_stat_cb);
          requisicoes_pendentes();
          Serial.printf("\n[DOCKER] Clique detectado de forma nativa:\n");
          Serial.printf("ID do Container:   %s\n", container_id);
          
          // Aqui você já pode chamar sua API para dar Start/Stop usando o ID puro
          // ex: enviar_comando_api(container_id, "stop");
      }
      
    }
}

static void cont_stat_cb(JsonDocument& doc) {
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

  Serial.println("\n--- MÉTRICAS DO CONTAINER ---");
  Serial.printf("ID: %s\n", id_container.c_str());
  Serial.printf("CPU Uso: %s (CPUs Disponiveis: %d)\n", cpu_percent.c_str(), cpu_online);
  Serial.printf("RAM Uso: %s de %s (%s)\n", ram_usage.c_str(), ram_limit.c_str(), ram_percent.c_str());
  Serial.println("-----------------------------");
}