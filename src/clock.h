#pragma once

#ifndef TINY__CLOCK_H
#define TINY__CLOCK_H

#include <stdint.h>
#include <time.h>

#include "draw.h"
#include "module.h"
#include "wayland.h"

typedef struct {
  char text[64];
} clock_state_t;

static void tiny__clock_update(bar_module_t *m) {
  clock_state_t *s = (clock_state_t *)m->state;

  time_t t = time(NULL);
  struct tm tm;
  localtime_r(&t, &tm);

  strftime(s->text, sizeof(s->text), "%Y-%m-%d %H:%M:%S", &tm);
  m->width = strlen(s->text) * FONT_ADV;
}

static void tiny__clock_draw(void *data, bar_module_t *m, uint32_t x,
                             uint32_t y) {
  TINY__Application *app = (TINY__Application *)data;
  tiny__draw_text(app->shm_data, app->width, app->height, x, y,
                  ((clock_state_t *)m->state)->text);
}

#endif // !TINY__CLOCK_H
