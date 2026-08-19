#ifndef LOGGER_H
#define LOGGER_H

#include <Arduino.h>

// Cores ANSI
#define LOG_COLOR_RESET   "\033[0m"
#define LOG_COLOR_INFO    "\033[32m" // Verde
#define LOG_COLOR_WARN    "\033[33m" // Amarelo
#define LOG_COLOR_ERROR   "\033[31m" // Vermelho
#define LOG_COLOR_DEBUG   "\033[36m" // Ciano

/* 
 * TRUQUE DE INDENTAÇÃO:
 * Criamos uma string longa de pontos. O formatador %.*s vai "cortar" essa string.
 * Se o nível for 1, ele imprime 2 pontos. Se for 2, imprime 4 pontos, etc.
 */
#define LOG_PAD_DOTS "...................." // Suporta até 10 níveis

// Macro Base (A verdadeira "engine" do log)
#define LOG_BASE(color, level_name, indent, tag, fmt, ...) \
    Serial.printf("%s[%lu] [%s] [%s] (%s) %s -> %.*s" fmt "%s\n", \
    color, millis(), level_name, tag, pcTaskGetName(NULL), __func__, (indent)*2, LOG_PAD_DOTS, ##__VA_ARGS__, LOG_COLOR_RESET)

// ==========================================
// NÍVEL 0 (Sem indentação - Início de evento)
// ==========================================
#define LOG_INFO(tag, fmt, ...)  LOG_BASE(LOG_COLOR_INFO,  "INFO", 0, tag, fmt, ##__VA_ARGS__)
#define LOG_WARN(tag, fmt, ...)  LOG_BASE(LOG_COLOR_WARN,  "WARN", 0, tag, fmt, ##__VA_ARGS__)
#define LOG_ERROR(tag, fmt, ...) LOG_BASE(LOG_COLOR_ERROR, "ERRO", 0, tag, fmt, ##__VA_ARGS__)
#define LOG_DEBUG(tag, fmt, ...) LOG_BASE(LOG_COLOR_DEBUG, "DBUG", 0, tag, fmt, ##__VA_ARGS__)

// ==========================================
// NÍVEL 1 (.. 2 pontos)
// ==========================================
#define LOG_INFO_1(tag, fmt, ...)  LOG_BASE(LOG_COLOR_INFO,  "INFO", 1, tag, fmt, ##__VA_ARGS__)
#define LOG_WARN_1(tag, fmt, ...)  LOG_BASE(LOG_COLOR_WARN,  "WARN", 1, tag, fmt, ##__VA_ARGS__)
#define LOG_ERROR_1(tag, fmt, ...) LOG_BASE(LOG_COLOR_ERROR, "ERRO", 1, tag, fmt, ##__VA_ARGS__)
#define LOG_DEBUG_1(tag, fmt, ...) LOG_BASE(LOG_COLOR_DEBUG, "DBUG", 1, tag, fmt, ##__VA_ARGS__)

// ==========================================
// NÍVEL 2 (.... 4 pontos - Subtarefas)
// ==========================================
#define LOG_INFO_2(tag, fmt, ...)  LOG_BASE(LOG_COLOR_INFO,  "INFO", 2, tag, fmt, ##__VA_ARGS__)
#define LOG_WARN_2(tag, fmt, ...)  LOG_BASE(LOG_COLOR_WARN,  "WARN", 2, tag, fmt, ##__VA_ARGS__)
#define LOG_ERROR_2(tag, fmt, ...) LOG_BASE(LOG_COLOR_ERROR, "ERRO", 2, tag, fmt, ##__VA_ARGS__)
#define LOG_DEBUG_2(tag, fmt, ...) LOG_BASE(LOG_COLOR_DEBUG, "DBUG", 2, tag, fmt, ##__VA_ARGS__)

#endif