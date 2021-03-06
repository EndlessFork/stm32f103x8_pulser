/*

simple font for 7-segment displays.

Bit assignment is 0bpgfedcba (i.e. msb is the point, lsb is seg a, which is top)
  -a-
 f   b
  -g-
 e   c
  -d-  p

*/

#ifndef _LED_FONT_H_
#define _LED_FONT_H_

#include <stdint.h>
// definition for the displayable ASCII chars
const uint8_t LED_FONT[] = {
  0b00000000, // <space>
  0b10000110, // !
  0b00100010, // "
  0b01111110, // #
  0b01101101, // $
  0b00000000, // %
  0b00000000, // &
  0b00000010, // '
  0b00110000, // (
  0b00000110, // )
  0b01100011, // *
  0b00000000, // +
  0b00000100, // ,
  0b01000000, // -
  0b10000000, // .
  0b01010010, // /
  0b00111111, // 0
  0b00000110, // 1
  0b01011011, // 2
  0b01001111, // 3
  0b01100110, // 4
  0b01101101, // 5
  0b01111101, // 6
  0b00100111, // 7
  0b01111111, // 8
  0b01101111, // 9
  0b00000000, // :
  0b00000000, // ;
  0b00000000, // <
  0b01001000, // =
  0b00000000, // >
  0b11010011, // ?
  0b01111011, // @
  0b01110111, // A
  0b01111100, // B
  0b00111001, // C
  0b01011110, // D
  0b01111001, // E
  0b01110001, // F
  0b00111101, // G
  0b01110100, // H
  0b00000110, // I
  0b00011110, // J
  0b01110101, // K
  0b00111000, // L
  0b00010101, // M
  0b01010100, // N
  0b01011100, // O
  0b01110011, // P
  0b01100111, // Q
  0b01010000, // R
  0b01101101, // S
  0b01111000, // T
  0b00011100, // U
  0b00101010, // V
  0b00011101, // W
  0b01110110, // X
  0b01101110, // Y
  0b01011011, // Z
  0b00111001, // [
  0b01100100, // \.
  0b00001111, // ]
  0b00000000, // ^
  0b00001000, // _
  0b00100000, // `
  0b01110111, // a
  0b01111100, // b
  0b01011000, // c
  0b01011110, // d
  0b01111001, // e
  0b01110001, // f
  0b00111101, // g
  0b01110100, // h
  0b00000110, // i
  0b00001110, // j
  0b01110101, // k
  0b00111000, // l
  0b01010101, // m
  0b01010100, // n
  0b01011100, // o
  0b01110011, // p
  0b01100111, // q
  0b01010000, // r
  0b01101101, // s
  0b01111000, // t
  0b00011100, // u
  0b00101010, // v
  0b00011101, // w
  0b01110110, // x
  0b01101110, // y
  0b01011011, // z
  0b01000110, // {
  0b00110000, // |
  0b01110000, // }
  0b00000001, // ~
};

#endif
