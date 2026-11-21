#pragma once
#include <lvgl.h>

enum AlertaCategoria {
  ALERTA_CAT_CPU = 0,
  ALERTA_CAT_RAM,
  ALERTA_CAT_TEMP,
  ALERTA_CAT_DOCKER,
  ALERTA_CAT_API,
  ALERTA_CAT_MAX
};

void db_alertas_init();
void lv_info_alertas(bool show_spinner = true);
void db_alertas_atualizar_estado(AlertaCategoria cat,
                                 bool ativo,
                                 const char *chave_estado,
                                 const char *nivel,
                                 lv_color_t cor,
                                 const char *mensagem);
