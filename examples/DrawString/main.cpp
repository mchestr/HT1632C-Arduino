#include "main.hpp"

void setup() {
  Serial.begin(115200);
  display.init();
  display.pwm(15);
}

void loop() {
  display.clear();
  display.string(5, 3, ("Time:" + String(millis() / 1000) + "s").c_str(),
                 &font_6x8, GREEN, BLACK);
  display.sendFrame();
}
