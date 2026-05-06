#pragma once
#ifndef TINY__BLOCKS_H
#define TINY__BLOCKS_H

#include <stdint.h>

#include "draw.h"
#include "module.h"
#include "wayland.h"

typedef struct {
  char text[64];
} blocks_state_t;

static void tiny__blocks_update(bar_module_t *m) {
  blocks_state_t *s = (blocks_state_t *)m->state;

  m->width = strlen(s->text) * FONT_ADV;
}

static void tiny__blocks_draw(void *data, bar_module_t *m, uint32_t x,
                              uint32_t y) {
  TINY__Application *app = (TINY__Application *)data;
  tiny__draw_text(app->shm_data, app->width, app->height, x, y,
                  ((blocks_state_t *)m->state)->text);
}

#endif // !TINY__BLOCKS_H
