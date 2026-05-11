#pragma once
#ifndef TINY__BLOCKS_H
#define TINY__BLOCKS_H

#include <stdint.h>

#include "draw.h"
#include "module.h"
#include "wayland.h"

typedef struct {
  char text[64];
  uint8_t bat_detected;
  uint8_t percent;
  uint8_t charging;
} blocks_state_t;

static void tiny__blocks_update(bar_module_t *m) {
  blocks_state_t *s = (blocks_state_t *)m->state;
  if (s->bat_detected) {
    m->width = strlen(s->text) * FONT_ADV + BATTERY_W;
  } else {
    m->width = strlen(s->text) * FONT_ADV;
  }
}

static void tiny__blocks_draw(void *data, bar_module_t *m, uint32_t x,
                              uint32_t y) {
  TINY__Application *app = (TINY__Application *)data;
  blocks_state_t *s = (blocks_state_t *)m->state;

  tiny__draw_text(app->shm_data, app->width, app->height, x, y, s->text);
  if (s->bat_detected) {
    // todo: xy is wrong probs
    tiny__draw_battery(app->shm_data, app->width, app->height,
                       (m->width - BATTERY_W) + x, y, s->percent, s->charging);
  }
}

#endif // !TINY__BLOCKS_H
