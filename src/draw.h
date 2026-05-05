#pragma once

#ifndef TINY__DRAW_H
#define TINY__DRAW_H

#include <stdint.h>
#include <string.h>

#include "font.h"

#define FONT_SCALE 2
#define FONT_ADV (6 * FONT_SCALE)

// Pre-scaled font storage: 11 chars, each 5*FONT_SCALE x 5*FONT_SCALE pixels
static uint32_t scaled_font[13][5 * FONT_SCALE][5 * FONT_SCALE];

// Build the scaled font map at program start
static void tiny__build_scaled_font() {
  for (int idx = 0; idx < 13; idx++) {
    for (int r = 0; r < 5; r++) {
      uint8_t row = font[idx][r];
      for (int c = 0; c < 5; c++) {
        uint32_t color = (row & (1 << (4 - c))) ? 0xFFFFFFFF : 0;
        // Fill scaled pixels
        for (int dy = 0; dy < FONT_SCALE; dy++)
          for (int dx = 0; dx < FONT_SCALE; dx++)
            scaled_font[idx][r * FONT_SCALE + dy][c * FONT_SCALE + dx] = color;
      }
    }
  }
}

// Draw a character using memcpy for maximum speed
static void tiny__draw_char(void *shm_data, uint32_t width, uint32_t height,
                            int x, int y, char c) {

  int idx;
  if (c >= '0' && c <= '9') {
    idx = c - '0'; // '0'-'9' -> 0-9
  } else if (c == ':') {
    idx = 10;
  } else if (c == '[') {
    idx = 11;
  } else if (c == ']') {
    idx = 12;
  } else {
    return; // unsupported char, do nothing
  }

  uint32_t *fb = (uint32_t *)shm_data;

  for (int r = 0; r < 5 * FONT_SCALE; r++) {
    int py = y + r;
    if ((unsigned)py >= height)
      continue; // clip vertically

    uint32_t *row_ptr = fb + py * width + x;
    int copy_width = 5 * FONT_SCALE;

    // Clip horizontally
    if (x < 0) {
      row_ptr -= x;    // move pointer inside framebuffer
      copy_width += x; // reduce copy width
    }
    if ((unsigned)(x + copy_width) > width) {
      copy_width = width - x;
    }
    if (copy_width <= 0)
      continue;

    memcpy(row_ptr, scaled_font[idx][r], copy_width * sizeof(uint32_t));
  }
}

static void tiny__clear_char(void *shm_data, uint32_t width, uint32_t height,
                             int x, int y) {
  uint32_t *fb = (uint32_t *)shm_data;

  for (int r = 0; r < 5 * FONT_SCALE; r++) {
    int py = y + r;
    if ((unsigned)py >= height)
      continue; // clip vertically

    int start_x = x;
    int char_width = 5 * FONT_SCALE;

    // Clip horizontally
    if (start_x < 0) {
      char_width += start_x;
      start_x = 0;
    }
    if ((unsigned)(start_x + char_width) > width) {
      char_width = width - start_x;
    }
    if (char_width <= 0)
      continue;

    uint32_t *row_ptr = fb + py * width + start_x;

    // Invert each pixel
    for (int i = 0; i < char_width; i++) {
      row_ptr[i] = 0;
    }
  }
}

// Draw a string of text
static void tiny__draw_text(void *shm_data, uint32_t width, uint32_t height,
                            int x, int y, const char *s) {
  int cursor = x;
  for (int i = 0; s[i]; i++) {
    tiny__draw_char(shm_data, width, height, cursor, y, s[i]);
    cursor += FONT_ADV;
  }
}

// Clear a string of text
static void tiny__clear_text(void *shm_data, uint32_t width, uint32_t height,
                             int x, int y, const char *s) {
  int cursor = x;
  for (int i = 0; s[i]; i++) {
    tiny__clear_char(shm_data, width, height, cursor, y);
    cursor += FONT_ADV;
  }
}

#endif // TINY__DRAW_H
