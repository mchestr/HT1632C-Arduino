#include <HT1632C.h>

const uint16_t font_4x6_sym_data[3][4] = {
    {0x00, 0x00, 0x00, 0x00},  // 0x00
    {0x18, 0x26, 0x19, 0x06},  // 0x01
    {0x06, 0x19, 0x26, 0x18},  // 0x02
};
const uint8_t font_4x6_sym_metric[3][2] = {
    {2, 1},
    {0, 0},
    {0, 0},
};
const FontInfo font_4x6_sym = {
    4, 6, 0, 2, font_4x6_sym_data[0], font_4x6_sym_metric[0]};
