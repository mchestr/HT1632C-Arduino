#include "HT1632C.hpp"

void abortIfInit(bool isInit) {
  if (isInit) {
    Serial.println(F("Display already initialized, unable to set value"));
    abort();
  }
}

HT1632C::HT1632C(const uint8_t num_panels, const uint8_t rotation)
    : num_panels(num_panels), rotation(rotation & 3), clk_pin(D1), cs_pin(D2) {}

HT1632C::HT1632C(const uint8_t num_panels, const uint8_t rotation,
                 const uint8_t clk_pin, const uint8_t cs_pin)
    : num_panels(num_panels),
      rotation(rotation & 3),
      clk_pin(clk_pin),
      cs_pin(cs_pin) {}

HT1632C::~HT1632C() {
  free(framebuffer);
}

void HT1632C::setChipHeight(uint8_t chip_height) {
  abortIfInit(isInit);
  this->chip_height = chip_height;
}

void HT1632C::setChipWidth(uint8_t chip_width) {
  abortIfInit(isInit);
  this->chip_width = chip_width;
}

void HT1632C::setChipsPerPanel(uint8_t chips_per_panel) {
  abortIfInit(isInit);
  this->chips_per_panel = chips_per_panel;
}

void HT1632C::setColors(uint8_t colors) {
  abortIfInit(isInit);
  this->colors = colors;
}

void HT1632C::setFrequency(uint32_t frequency) {
  abortIfInit(isInit);
  this->frequency = frequency;
}

void HT1632C::setPanelWidth(uint8_t width) {
  abortIfInit(isInit);
  this->panel_width = width;
}

void HT1632C::setPanelHeight(uint8_t height) {
  abortIfInit(isInit);
  this->panel_height = height;
}

int HT1632C::init() {
  width = panel_width * num_panels;
  height = panel_height;
  color_size = chip_width * chip_height / 8;
  chip_size = (color_size * colors) + 2;
  num_chips = chips_per_panel * num_panels;
  DEBUGF("Chip Size: %d\n", chip_size);
  framebuffer = (uint8_t*)malloc(num_chips * chip_size);
  if (!framebuffer) {
    return 3;
  }

  SPI.begin();
  SPI.setFrequency(frequency);

  pinMode(clk_pin, OUTPUT);
  pinMode(cs_pin, OUTPUT);

  // init display
  sendCmd(CS_ALL, COMMAND_CODE::SYSDIS);
  sendCmd(CS_ALL,
          (chip_height <= 8) ? COMMAND_CODE::COMS00 : COMMAND_CODE::COMS01);
  sendCmd(CS_ALL, COMMAND_CODE::MSTMD);
  sendCmd(CS_ALL, COMMAND_CODE::RCCLK);
  sendCmd(CS_ALL, COMMAND_CODE::SYSON);
  sendCmd(CS_ALL, COMMAND_CODE::LEDON);
  sendCmd(CS_ALL, COMMAND_CODE::BLOFF);
  sendCmd(CS_ALL, COMMAND_CODE::PWM);

  clear();
  sendFrame();
  return 0;
}

int HT1632C::getWidth() { return (rotation & 1) ? height : width; }

int HT1632C::getHeight() { return (rotation & 1) ? width : height; }

void HT1632C::ledOn() { sendCmd(CS_ALL, COMMAND_CODE::LEDON); }

void HT1632C::ledOff() { sendCmd(CS_ALL, COMMAND_CODE::LEDOFF); }

void HT1632C::blinkOn() { sendCmd(CS_ALL, COMMAND_CODE::BLON); }

void HT1632C::blinkOff() { sendCmd(CS_ALL, COMMAND_CODE::BLOFF); }

void HT1632C::pwm(const uint8_t value) {
  sendCmd(CS_ALL, COMMAND_CODE::PWM | (value & 0x0f));
}

void HT1632C::clear() {
  // clear buffer
  memset(framebuffer, 0, num_chips * chip_size);
  for (int i = 0; i < num_chips; ++i) {
    *framebufferPtr(i, 0) = ID_CODE::WR << (8 - ID_LEN);
  }
  // reset clipping
  clipReset();
}

void HT1632C::sendFrame() {
  for (int chip = 0; chip < num_chips; ++chip) {
    chipSelect(chip + 1);
    for (uint8_t addr = 0; addr < chip_size; addr++)
      SPI.write(*framebufferPtr(chip, addr));
    chipSelect(CS_NONE);
  }
}

void HT1632C::clip(const int x0, const int y0, const int x1, const int y1) {
  clipX0 = (x0 >= 0) ? x0 : 0;
  clipY0 = (y0 >= 0) ? y0 : 0;
  clipX1 = (x1 >= 0) ? x1 : getWidth();
  clipY1 = (y1 >= 0) ? y1 : getHeight();
}

void HT1632C::plot(const int rx, const int ry, const uint8_t color) {
  // clipping
  if (rx < clipX0 || rx >= clipX1 || ry < clipY0 || ry >= clipY1) return;

  // rotation
  int x = (rotation & 1) ? ry : rx;
  if ((rotation & 1) != (rotation >> 1)) x = (width - 1) - x;
  int y = (rotation & 1) ? rx : ry;
  if (rotation & 2) y = (height - 1) - y;

  const int xc = x / chip_width;
  const int yc = y / chip_height;
  const int chip = xc + (xc & 0xfffe) + (yc * 2);

  const int xb = (x % chip_width) + (chip_height / 8) - 1;
  const int yb = (y % chip_height) + PANEL_HEADER_BITS;
  int addr = xb + (yb / 8);
  const uint8_t bitval = 128 >> (yb & 7);

  // first color
  updateFramebuffer(chip, addr, (color & 1), bitval);
  if (addr <= 1 && bitval > 2) {
    // special case: first bits must are 'wrapped' to the end
    updateFramebuffer(chip, addr + chip_size - 2, (color & 1), bitval);
  }
  // other colors
  for (int i = 1; i < colors; ++i) {
    addr += color_size;
    updateFramebuffer(chip, addr, (color & (1 << i)), bitval);
  }
}

void HT1632C::line(const int x0, const int y0, const int x1, const int y1,
                   const uint8_t color) {
  const int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
  const int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
  int err = dx + dy, e2; /* error value e_xy */

  int x = x0, y = y0;
  for (;;) {
    plot(x, y, color);
    if (x == x1 && y == y1) break;
    e2 = 2 * err;
    if (e2 >= dy) {
      err += dy;
      x += sx;
    } /* e_xy+e_x > 0 */
    if (e2 <= dx) {
      err += dx;
      y += sy;
    } /* e_xy+e_y < 0 */
  }
}

void HT1632C::box(int x0, int y0, int x1, int y1, const uint8_t color) {
  {
    int tmp;
    if (x1 < x0) {
      tmp = x0;
      x0 = x1;
      x1 = tmp;
    };
    if (y1 < y0) {
      tmp = y0;
      y0 = y1;
      y1 = tmp;
    };
  }

  for (int y = y0; y <= y1; ++y) {
    for (int x = x0; x <= x1; ++x) {
      plot(x, y, color);
    }
  }
}

int HT1632C::character(const int x, const int y, const char c,
                       const FontInfo* font, const uint8_t color,
                       const uint8_t bg) {
  const int width = font->width;
  const int height = font->height;
  const int map_start = font->map_start;
  const int map_end = font->map_end;

  if (c < map_start) return 0;
  if (c > map_end) return 0;

  uint16_t const* addr = font->data + (c - map_start) * width;
  for (int col = 0; col < width; ++col) {
    uint16_t dots = addr[col];
    for (int row = height - 1; row >= 0; --row) {
      if (dots & 1)
        plot(x + col, y + row, color);
      else if (bg != TRANSPARENT)
        plot(x + col, y + row, bg);
      dots >>= 1;
    }
  }

  return x + width;
}

int HT1632C::characterMetric(const int x, const int y, const char c,
                             const FontInfo* font, const uint8_t color,
                             const uint8_t bg) {
  const int width = font->width;
  const int height = font->height;
  const int map_start = font->map_start;
  const int map_end = font->map_end;

  if (c < map_start) return 0;
  if (c > map_end) return 0;

  uint16_t const* addr = font->data + (c - map_start) * width;
  uint8_t const* metric_addr = font->metric + 2 * (c - map_start);
  const int left = metric_addr[0];
  const int right = metric_addr[1];
  for (int col = left; col < width - right; ++col) {
    uint16_t dots = addr[col];
    for (int row = height - 1; row >= 0; --row) {
      if (dots & 1)
        plot(x + col - left, y + row, color);
      else if (bg != TRANSPARENT)
        plot(x + col - left, y + row, bg);
      dots >>= 1;
    }
  }

  int col = width - right;
  if (bg != TRANSPARENT)
    for (int row = height - 1; row >= 0; --row)
      plot(x + col - left, y + row, bg);

  return x + width - left - right + 1;
}

int HT1632C::string(const int x, const int y, const char* s,
                    const FontInfo* font, const uint8_t color,
                    const uint8_t bg) {
  int p;
  for (p = x; *s; ++s) {
    p = character(p, y, *s, font, color, bg);
  }
  return p;
}

int HT1632C::stringMetric(const int x, const int y, const char* s, const FontInfo* font,
                 const uint8_t color, const uint8_t bg) {
  int p;
  for (p = x; *s; ++s) {
    p = characterMetric(p, y, *s, font, color, bg);
  }
  return p;
}

void HT1632C::sendCmd(const uint8_t chip, const uint8_t cmd) {
  uint16_t data = ((ID_CODE::CMD << 8) | cmd) << 5;
  chipSelect(chip);
  SPI.write16(data);
  chipSelect(CS_NONE);
}

void HT1632C::chipSelect(const uint8_t cs) {
  if (cs == CS_ALL) {
    digitalWrite(cs_pin, 0);
    clkPulse(num_chips);
  } else if (cs == CS_NONE) {
    digitalWrite(cs_pin, 1);
    clkPulse(num_chips);
  } else {
    digitalWrite(cs_pin, 0);
    clkPulse(1);
    digitalWrite(cs_pin, 1);
    clkPulse(cs - 1);
  }
}

void HT1632C::clkPulse(int num) {
  while (num--) {
    digitalWrite(clk_pin, 1);
    digitalWrite(clk_pin, 0);
  }
}

void HT1632C::updateFramebuffer(const int chip, const int addr,
                                const uint8_t target_bitval,
                                const uint8_t pixel_bitval) {
  uint8_t* const v = framebufferPtr(chip, addr);
  if (target_bitval)
    *v |= pixel_bitval;
  else
    *v &= ~pixel_bitval;
}

uint8_t* HT1632C::framebufferPtr(uint8_t chip, uint8_t addr) {
  return framebuffer + (chip * chip_size + addr);
}

void HT1632C::debugFramebuffer() {
  for (int chip = 0; chip < num_chips; chip++) {
    DEBUGF("Chip %d: ", chip);
    uint8_t* ptr = framebufferPtr(chip, 0);
    for (int i = 0; i < chip_size; i++) {
      Serial.print(*(ptr + i), HEX);
      if (i != chip_size - 1) {
        DEBUGF(",");
      }
    }
    DEBUGF("\n");
  }
}
