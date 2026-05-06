#pragma once

#ifndef TINY__MODULE_H
#define TINY__MODULE_H

#define LEFT_MARGIN 10
#define RIGHT_MARGIN 10

#include <stdint.h>

typedef struct bar_module bar_module_t;

struct bar_module {
  void (*update)(bar_module_t *m);
  void (*draw)(void *data, bar_module_t *m, uint32_t x, uint32_t y);
  int width;
  void *state;
};

static void tiny__draw_modules(void *data, bar_module_t *modules,
                               int module_count, uint32_t width) {
  uint32_t total_width = 0;

  for (int i = 0; i < module_count; i++) {
    total_width += modules[i].width;
  }

  uint32_t usable_width = width - LEFT_MARGIN - RIGHT_MARGIN;

  if (module_count <= 1) {
    if (module_count == 1) {
      modules[0].draw(data, &modules[0], LEFT_MARGIN, 2);
    }
    return;
  }

  uint32_t gap = (usable_width - total_width) / (module_count - 1);

  uint32_t x = LEFT_MARGIN;

  for (int i = 0; i < module_count; i++) {
    modules[i].draw(data, &modules[i], x, 2);
    x += modules[i].width + gap;
  }
}

#endif // !TINY__MODULE_H
