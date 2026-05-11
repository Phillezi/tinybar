#pragma once

#ifndef TINY__DRAW_H
#define TINY__DRAW_H

#include <stdint.h>
#include <string.h>

#include "font.h"

#define FONT_SCALE 2
#define FONT_ADV (6 * FONT_SCALE)

#define BATTERY_W (22 * FONT_SCALE)
#define BATTERY_H (9 * FONT_SCALE)

#define TINY_COLOR_WHITE 0xFFFFFFFF
#define TINY_COLOR_ZERO 0x00000000

// Pre-scaled font storage: 11 chars, each 5*FONT_SCALE x 5*FONT_SCALE pixels
static uint32_t scaled_font[FONT_CHARS_LEN][FONT_CHAR_HEIGHT * FONT_SCALE]
                           [FONT_CHAR_HEIGHT * FONT_SCALE];

// Build the scaled font map at program start
static void tiny__build_scaled_font() {
  for (int idx = 0; idx < FONT_CHARS_LEN; idx++) {
    for (int r = 0; r < FONT_CHAR_HEIGHT; r++) {
      uint8_t row = font[idx][r];
      for (int c = 0; c < FONT_CHAR_HEIGHT; c++) {
        uint32_t color =
            (row & (1 << (4 - c))) ? TINY_COLOR_WHITE : TINY_COLOR_ZERO;
        // Fill scaled pixels
        for (int dy = 0; dy < FONT_SCALE; dy++)
          for (int dx = 0; dx < FONT_SCALE; dx++)
            scaled_font[idx][r * FONT_SCALE + dy][c * FONT_SCALE + dx] = color;
      }
    }
  }
}

static inline void tiny__set_pixel(void *shm_data, uint32_t width,
                                   uint32_t height, int x, int y,
                                   uint32_t color) {

  if ((unsigned)x >= width || (unsigned)y >= height)
    return;

  uint32_t *fb = (uint32_t *)shm_data;

  fb[y * width + x] = color;
}

static void tiny__fill_rect(void *shm_data, uint32_t width, uint32_t height,
                            int x, int y, int w, int h, uint32_t color) {

  for (int iy = 0; iy < h; iy++) {

    int py = y + iy;

    if ((unsigned)py >= height)
      continue;

    for (int ix = 0; ix < w; ix++) {

      int px = x + ix;

      if ((unsigned)px >= width)
        continue;

      tiny__set_pixel(shm_data, width, height, px, py, color);
    }
  }
}

static void tiny__draw_rect_outline(void *shm_data, uint32_t width,
                                    uint32_t height, int x, int y, int w, int h,
                                    uint32_t color) {

  for (int ix = 0; ix < w; ix++) {
    tiny__set_pixel(shm_data, width, height, x + ix, y, color);
    tiny__set_pixel(shm_data, width, height, x + ix, y + h - 1, color);
  }

  for (int iy = 0; iy < h; iy++) {
    tiny__set_pixel(shm_data, width, height, x, y + iy, color);
    tiny__set_pixel(shm_data, width, height, x + w - 1, y + iy, color);
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

static void tiny__draw_battery(void *shm_data, uint32_t width, uint32_t height,
                               int x, int y, uint8_t percent,
                               uint8_t charging) {

  if (percent < 0)
    percent = 0;

  if (percent > 100)
    percent = 100;

  const int body_w = BATTERY_W;
  const int body_h = BATTERY_H;

  const int terminal_w = 2 * FONT_SCALE;
  const int terminal_h = 3 * FONT_SCALE;

  //
  // Battery shell
  //

  tiny__draw_rect_outline(shm_data, width, height, x, y, body_w, body_h,
                          TINY_COLOR_WHITE);

  //
  // Terminal nub
  //

  tiny__fill_rect(shm_data, width, height, x + body_w,
                  y + (body_h - terminal_h) / 2, terminal_w, terminal_h,
                  TINY_COLOR_WHITE);

  //
  // Fill bar
  //

  int inner_x = x + FONT_SCALE;
  int inner_y = y + FONT_SCALE;

  int inner_w = body_w - (2 * FONT_SCALE);
  int inner_h = body_h - (2 * FONT_SCALE);

  int fill_w = (inner_w * percent) / 100;

  tiny__fill_rect(shm_data, width, height, inner_x, inner_y, fill_w, inner_h,
                  TINY_COLOR_WHITE);

  //
  // Percentage text
  //

  if (!charging) {

    char txt[4];
    int len;

    if (percent == 100) {
      txt[0] = '1';
      txt[1] = '0';
      txt[2] = '0';
      txt[3] = '\0';
      len = 3;

    } else if (percent >= 10) {
      txt[0] = '0' + (percent / 10);
      txt[1] = '0' + (percent % 10);
      txt[2] = '\0';
      len = 2;

    } else {
      txt[0] = '0' + percent;
      txt[1] = '\0';
      len = 1;
    }

    int text_w = (len * FONT_ADV) - FONT_SCALE;

    int tx = x + (body_w - text_w) / 2;
    int ty = y + ((body_h - (5 * FONT_SCALE)) / 2);

    //
    // background behind text
    //

    tiny__fill_rect(shm_data, width, height, tx - FONT_SCALE / 2, ty,
                    text_w + FONT_SCALE, 5 * FONT_SCALE, TINY_COLOR_ZERO);

    tiny__draw_text(shm_data, width, height, tx, ty, txt);
  }

  //
  // Charging bolt overlay
  //

  if (charging) {

    int bx = x + body_w / 2;
    int by = y + body_h / 2;

    int s = FONT_SCALE;

    tiny__fill_rect(shm_data, width, height, bx, by - (2 * s), s, 2 * s,
                    TINY_COLOR_ZERO);

    tiny__fill_rect(shm_data, width, height, bx - s, by, s, 2 * s,
                    TINY_COLOR_ZERO);

    tiny__fill_rect(shm_data, width, height, bx, by, s, 2 * s,
                    TINY_COLOR_WHITE);

    tiny__fill_rect(shm_data, width, height, bx + s, by - (2 * s), s, 2 * s,
                    TINY_COLOR_WHITE);
  }
}

#endif // TINY__DRAW_H
