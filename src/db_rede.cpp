#include "ui/ui.h"
#include "db_rede.h"
#include <Esp.h>

extern "C" void action_resetar(lv_event_t *e) {
    // TODO: Implement action resetar here
    ESP.restart();
}