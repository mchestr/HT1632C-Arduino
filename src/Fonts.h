#include <inttypes.h>

typedef struct {
  const uint8_t width;
  const uint8_t height;
  const uint8_t map_start;
  const uint8_t map_end;
  const uint16_t* data;
  const uint8_t* metric;
} FontInfo;

int characterWidth(const char c, const FontInfo* font);
int stringWidth(const char* s, const FontInfo* font);
int fontWidth(const FontInfo* font);
int fontHeight(const FontInfo* font);
