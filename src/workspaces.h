#pragma once
#ifndef TINY__WORKSPACES_H
#define TINY__WORKSPACES_H

#include <stdint.h>
#include <string.h>

#include "draw.h"
#include "module.h"
#include "wayland.h"

static void tiny__ws_update(bar_module_t *m) {
  ws_state_t *s = (ws_state_t *)m->state;
  // TODO: modify the state externally
  // snprintf(s->text, sizeof(s->text), "%s", workspaces);
  m->width = strlen(s->text) * FONT_ADV;
}

static void tiny__ws_draw(void *data, bar_module_t *m, uint32_t x, uint32_t y) {
  TINY__Application *app = (TINY__Application *)data;

  tiny__clear_text(app->shm_data, app->width, app->height, x, y,
                   ((ws_state_t *)m->state)->prev_text);

  tiny__draw_text(app->shm_data, app->width, app->height, x, y,
                  ((ws_state_t *)m->state)->text);
  strcpy(((ws_state_t *)m->state)->prev_text, ((ws_state_t *)m->state)->text);
}

#endif // !TINY__WORKSPACES_H
