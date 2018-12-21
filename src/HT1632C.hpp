#include <Arduino.h>
#include <SPI.h>
#include <inttypes.h>

#include "Commands.h"
#include "Fonts.h"

//
// constants
//
#define BLACK 0
#define GREEN 1
#define RED 2
#define ORANGE 3
#define TRANSPARENT 0xff

#ifdef DEBUG
#define DEBUGF(...) Serial.printf(__VA_ARGS__)
#else
#define DEBUGF(...)
#endif

class HT1632C {
 public:
  HT1632C(const uint8_t num_panels, const uint8_t rotation);
  HT1632C(const uint8_t num_panels, const uint8_t rotation,
          const uint8_t clk_pin, const uint8_t cs_pin);
  ~HT1632C();
  void setChipHeight(uint8_t chip_height);
  void setChipWidth(uint8_t chip_width);
  void setChipsPerPanel(uint8_t chips_per_panel);
  void setColors(uint8_t colors);
  void setFrequency(uint32_t frequency);
  void setCsClkDelay(uint8_t delay);
  void setPanelWidth(uint8_t width);
  void setPanelHeight(uint8_t height);

  int init();
  int getWidth();
  int getHeight();
  void ledOn();
  void ledOff();
  void blinkOn();
  void blinkOff();
  void pwm(const uint8_t value);
  void clear();
  void sendFrame();

  // Plotting functions
  void clip(const int x0, const int y0, const int x1, const int y1);
  void clipReset() { clip(-1, -1, -1, -1); };
  void plot(const int rx, const int ry, const uint8_t color);
  void line(const int x0, const int y0, const int x1, const int y1,
            const uint8_t color);
  void box(int x0, int y0, int x1, int y1, const uint8_t color);
  int character(const int x, const int y, const char c, const FontInfo* font,
                const uint8_t color, const uint8_t bg);
  int characterMetric(const int x, const int y, const char c, const FontInfo* font,
                const uint8_t color, const uint8_t bg);
  int string(const int x, const int y, const char* s, const FontInfo* font,
             const uint8_t color, const uint8_t bg);
  int stringMetric(const int x, const int y, const char* s, const FontInfo* font,
             const uint8_t color, const uint8_t bg);

 private:
  uint8_t num_panels;
  uint8_t rotation;
  uint8_t clk_pin;
  uint8_t cs_pin;
  bool isInit = false;

  uint8_t panel_width = 32;
  uint8_t panel_height = 16;
  uint8_t chip_height = 8;
  uint8_t chip_width = 16;
  uint8_t colors = 2;
  uint8_t chips_per_panel = 4;
  uint32_t frequency = 2560000;

  uint16_t width;
  uint8_t height;
  uint8_t num_chips;
  uint8_t color_size;
  uint16_t chip_size;

  int clipX0;
  int clipX1;
  int clipY0;
  int clipY1;

  void sendCmd(const uint8_t chip, const uint8_t cmd);
  void chipSelect(const uint8_t cs);
  void clkPulse(int num);

  uint8_t* framebuffer;
  void updateFramebuffer(const int chip, const int addr,
                         const uint8_t target_bitval,
                         const uint8_t pixel_bitval);
  inline uint8_t* framebufferPtr(uint8_t chip, uint8_t addr);

  void debugFramebuffer();
};
