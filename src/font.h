#pragma once

#ifndef TINY__FONT_H
#define TINY__FONT_H

#include <stdint.h>

#define FONT_CHARS_LEN 13
#define FONT_CHAR_HEIGHT 5

// 5x5 bitmap font for '0'-'9', ':', '[', ']'
static const uint8_t font[FONT_CHARS_LEN][FONT_CHAR_HEIGHT] = {
    // 0
    {0b01110, 0b10001, 0b10001, 0b10001, 0b01110},
    // 1
    {0b00100, 0b01100, 0b00100, 0b00100, 0b01110},
    // 2
    {0b01110, 0b10001, 0b00110, 0b01000, 0b11111},
    // 3
    {0b11110, 0b00001, 0b01110, 0b00001, 0b11110},
    // 4
    {0b10010, 0b10010, 0b11111, 0b00010, 0b00010},
    // 5
    {0b11111, 0b10000, 0b11110, 0b00001, 0b11110},
    // 6
    {0b01110, 0b10000, 0b11110, 0b10001, 0b01110},
    // 7
    {0b11111, 0b00010, 0b00100, 0b01000, 0b01000},
    // 8
    {0b01110, 0b10001, 0b01110, 0b10001, 0b01110},
    // 9
    {0b01110, 0b10001, 0b01111, 0b00001, 0b01110},
    // : (colon)
    {0b00000, 0b00100, 0b00000, 0b00100, 0b00000},
    // [ (left bracket)
    {0b01110, 0b01000, 0b01000, 0b01000, 0b01110},
    // ] (right bracket)
    {0b01110, 0b00010, 0b00010, 0b00010, 0b01110},
};
/*

#define ICONS_LEN 15
#define ICON_HEIGHT 5
#define ICON_INDEX_BAT_0 0

static const uint8_t icons[ICONS_LEN][ICON_HEIGHT] = {
    // bat_0
    {},
    // bat_5
    {},
    // bat_10
    {},
    // bat_15
    {},
    // bat_20
    {},
    // bat_30
    {},
    // bat_40
    {},
    // bat_50
    {},
    // bat_60
    {},
    // bat_70
    {},
    // bat_80
    {},
    // bat_90
    {},
    // bat_95
    {},
    // bat_100
    {},
    // bat_charging
    {},
};*/

#endif // !TINY__FONT_H
