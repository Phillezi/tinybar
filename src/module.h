#pragma once

#ifndef TINY__MODULE_H
#define TINY__MODULE_H

#define LEFT_MARGIN 10
#define RIGHT_MARGIN 10

#include <stdint.h>

typedef struct bar_module bar_module_t;

typedef struct {
  char text[64];
} clock_state_t;

typedef struct {
  char text[128];
  char prev_text[128];
} ws_state_t;

struct bar_module {
  void (*update)(bar_module_t *m);
  void (*draw)(void *data, bar_module_t *m, uint32_t x, uint32_t y);
  int width;
  void *state;
};

static void tiny__draw_modules(void *data, bar_module_t *modules,
                               int module_count, uint32_t width) {
  uint32_t x_left = LEFT_MARGIN;
  uint32_t x_right = width - RIGHT_MARGIN;

  for (int i = 0; i < module_count; i++) {
    if (i == module_count - 1) {
      x_right -= modules[i].width;
      modules[i].draw(data, &modules[i], x_right, 10);
    } else {
      modules[i].draw(data, &modules[i], x_left, 10);
      x_left += modules[i].width + 10;
    }
  }
}

#endif // !TINY__MODULE_H
