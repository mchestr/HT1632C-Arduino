# HT1632C - Arduino

This library is used to interface with an HT1632C chip used to control an LED matrix.

This library has only been tested using a 32x16 RED/GREEN LED matrix, and only 2 chained together in total (64x16 LEDs).

# Usage

See examples directory for more examples.

```
#include <Arduino.h>

#include <HT1632C.h>
#include "fonts.cpp"

HT1632C display(2, 0);

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
```

## Fonts

There are some example fonts in the `examples/Fonts` directory.
To define your own fonts you must define an array of mappings to each ASCII character you want to use.

### Example

To define a new font, first choose the dimension of the new font. For example to define a 6x8 font, define a character array
```
const uint16_t font_6x8_data[256][6] = {
    ...
}
```

To define a new capital T (hex 0x54), in the index 84 (0x54 = 84) of your array you would define the LED mapping.

```
const uint16_t font_6x8_data[256][6] = {
    ...
    {0x00, 0x80, 0x80, 0xFE, 0x80, 0x80}, // index 84
    ...
}
```

Where `{0x00, 0x80, 0x80, 0xFE, 0x80, 0x80}` where each byte maps to a column.

```
011111
000100
000100
000100
000100
000100
000100
000000
```


