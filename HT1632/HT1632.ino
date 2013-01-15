/***********************************************************************
 * demo16x24.c - Arduino demo program for Holtek HT1632 LED driver chip,
 *            As implemented on the Sure Electronics DE-DP016 display board
 *            (16*24 dot matrix LED module.)
 * Nov, 2008 by Bill Westfield ("WestfW")
 *   Copyrighted and distributed under the terms of the Berkeley license
 *   (copy freely, but include this notice of original author.)
 *
 * Dec 2010, FlorinC - adapted for 3216 display
 ***********************************************************************/

#include <WProgram.h>
#include "ht1632.h"
#include <avr/pgmspace.h>
#include "font3.h"
#include <math.h>

#define CHIP_MAX 8           //4 Chips per board
#define X_MAX    CHIP_MAX*8  //32 per board, 4 chips per board
#define Y_MAX    16

#define SHORTDELAY 20
#define DISPDELAY  40      // Each "display" lasts this long
#define LONGDELAY  1000    // This delay BETWEEN demos

#define BLACK  0
#define GREEN  1
#define RED    2
#define ORANGE 3

static const byte ht1632_data = 6;  // Data pin (pin 7 of display connector)
static const byte ht1632_wrclk = 7; // Write clock pin (pin 5 of display connector)
static const byte ht1632_cs = 8;    // Chip Select (pin 1 of display connnector)
static const byte ht1632_clk = 9;   // clock pin (pin 2 of display connector)

// our own copy of the "video" memory; 64 bytes for each of the screen quarters;
// each 64-element array maps 2 planes:
// indexes from 0 to 31 are allocated for green plane;
// indexes from 32 to 63 are allocated for red plane;
// when a bit is 1 in both planes, it is displayed as orange (green + red);
byte ht1632_shadowRam[64][CHIP_MAX] = {0};


/*
 * Function Name: ht1632_CLKPulse
 * Function Feature: enable CLK_74164 pin to output a clock pulse
 * Input Argument: void
 * Output Argument: void
 */
void ht1632_CLKPulse () //Output a clock pulse
{
    digitalWrite(ht1632_clk, HIGH);
    digitalWrite(ht1632_clk, LOW);
}


/*
 * Function Name: ht1632_pinA74164
 * Function Feature: enable pin A of 74164 to output 0 or 1
 * Input Argument: x: if x=1, 74164 outputs high. If x?1, 74164 outputs low.
 * Output Argument: void
 */
 void ht1632_pinA74164 (byte x) //Input a digital level to 74164
{
    digitalWrite(ht1632_cs, (x == 1 ? HIGH:LOW));
}


/*
 * Function Name: ht1632_chipSelect
 * Function Feature: enable HT1632C
 * Input Argument: select: HT1632C to be selected
 * If select=0, select none.
 * If s<0, select all.
 * Output Argument: void
 */
void ht1632_chipSelect (int select)
{
    byte tmp = 0;
    if (select < 0) //Enable all HT1632Cs
    {
        ht1632_pinA74164(0);
        for (tmp = 0; tmp < CHIP_MAX; tmp++)
            ht1632_CLKPulse();
    }
    else if (select == 0) //Disable all HT1632Cs
    {
        ht1632_pinA74164(1);
        for (tmp = 0; tmp < CHIP_MAX; tmp++)
            ht1632_CLKPulse();
    }
    else
    {
        ht1632_pinA74164(1);
        for (tmp = 0; tmp < CHIP_MAX; tmp++)
            ht1632_CLKPulse();
        ht1632_pinA74164(0);
        ht1632_CLKPulse();
        ht1632_pinA74164(1);
        tmp = 1;
        for (; tmp < select; tmp++)
            ht1632_CLKPulse();
    }
}


/*
 * ht1632_writebits
 * Write bits (up to 8) to h1632 on pins ht1632_data, ht1632_wrclk
 * Chip is assumed to already be chip-selected
 * Bits are shifted out from MSB to LSB, with the first bit sent
 * being (bits & firstbit), shifted till firsbit is zero.
 */
void ht1632_writebits (byte bits, byte firstbit)
{
    DEBUGPRINT(" ");
    while (firstbit) 
    {
        DEBUGPRINT((bits & firstbit ? "1":"0"));
        digitalWrite(ht1632_wrclk, LOW);
        if (bits & firstbit) 
            digitalWrite(ht1632_data, HIGH);
        else 
            digitalWrite(ht1632_data, LOW);
        digitalWrite(ht1632_wrclk, HIGH);
        firstbit >>= 1;
    }
}


/*
 * ht1632_sendCmd
 * Send a command to the ht1632 chip.
 */
static void ht1632_sendCmd (byte chipNo, byte command)
{
    ht1632_chipSelect(chipNo);
    ht1632_writebits(HT1632_ID_CMD, 1 << 2);  // send 3 bits of id: COMMMAND
    ht1632_writebits(command, 1 << 7);        // send the actual command
    ht1632_writebits(0, 1); 	            // one extra dont-care bit in commands.
    ht1632_chipSelect(0);
}


/*
 * ht1632_sendData
 * send a nibble (4 bits) of data to a particular memory location of the
 * ht1632.  The command has 3 bit ID, 7 bits of address, and 4 bits of data.
 *    Select 1 0 1 A6 A5 A4 A3 A2 A1 A0 D0 D1 D2 D3 Free
 * Note that the address is sent MSB first, while the data is sent LSB first!
 * This means that somewhere a bit reversal will have to be done to get
 * zero-based addressing of words and dots within words.
 */
static void ht1632_sendData (byte chipNo, byte address, byte data)
{
    ht1632_chipSelect(chipNo);
    ht1632_writebits(HT1632_ID_WR, 1 << 2);  // send ID: WRITE to RAM
    ht1632_writebits(address, 1 << 6); // Send address
    ht1632_writebits(data, 1 << 3); // send 4 bits of data
    ht1632_chipSelect(0);
}


/*
 * ht1632_clear
 * clear the display, and the shadow memory, and the snapshot
 * memory.  This uses the "write multiple words" capability of
 * the chipset by writing all 96 words of memory without raising
 * the ht1632_chipSelect signal.
 */
void ht1632_clear ()
{
    ht1632_chipSelect(-1); //select all chips
    ht1632_writebits(HT1632_ID_WR, 1 << 2);  // send ID: WRITE to RAM
    ht1632_writebits(0, 1 << 6); // Send address
    for (byte i = 0; i < 32; i++) // Clear entire display
        ht1632_writebits(0, 1 << 7); // send 8 bits of data
    ht1632_chipSelect(0);
}


/*
 * return the value of a pixel from the video memory (either BLACK, RED, GREEN, ORANGE);
 */
byte ht1632_getShadowRam (byte x, byte y)
{
    byte nQuarter = x/16 + (y > 7 ? 2:0);

    x = x % 16;
    y = y % 8;

    byte addr = (x << 1) + (y >> 2);
    byte bitval = 8 >> (y & 3);
    byte retVal = 0;
    byte val = (ht1632_shadowRam[addr][nQuarter] & bitval) ? 1:0;
    val += (ht1632_shadowRam[addr+32][nQuarter] & bitval) ? 2:0;
    return val;
}


void ht1632_setup ()
{
    pinMode(ht1632_cs, OUTPUT);
    digitalWrite(ht1632_cs, HIGH); 	// unselect (active low)
    pinMode(ht1632_wrclk, OUTPUT);
    pinMode(ht1632_data, OUTPUT);
    pinMode(ht1632_clk, OUTPUT);

    for (byte j = 1; j <= CHIP_MAX; j++)
    {
        ht1632_sendCmd(j, HT1632_CMD_SYSDIS);       // Disable system
        ht1632_sendCmd(j, HT1632_CMD_COMS00);
        ht1632_sendCmd(j, HT1632_CMD_MSTMD); 	// Master Mode
        ht1632_sendCmd(j, HT1632_CMD_RCCLK);        // HT1632C
        ht1632_sendCmd(j, HT1632_CMD_SYSON); 	// System on
        ht1632_sendCmd(j, HT1632_CMD_LEDON); 	// LEDs on
    }
    ht1632_clear();
    delay(LONGDELAY);
}


/*
 * ht1632_plot a point on the display, with the upper left hand corner
 * being (0,0), and the lower right hand corner being (31, 15);
 * parameter "color" could have one of the 4 values:
 * black (off), red, green or yellow;
 */
void ht1632_plot(byte x, byte y, byte color)
{
    if (x < 0 || x > X_MAX || y < 0 || y > Y_MAX)
        return;
    if (color != BLACK && color != GREEN && color != RED && color != ORANGE)
        return;
  
    byte nChip = 1 + x/16 + (y > 7 ? 2:0) + (x > 31 ? 2:0);
  
    x = x % 16;
    y = y % 8;
  
    byte addr = (x << 1) + (y >> 2);
    byte bitval = 8 >> (y & 3);  // compute which bit will need set
  
    switch (color)
    {
        case BLACK:
            // clear the bit in both planes;
            ht1632_shadowRam[addr][nChip-1] &= ~bitval;
            ht1632_sendData(nChip, addr, ht1632_shadowRam[addr][nChip-1]);
            addr = addr + 32;
            ht1632_shadowRam[addr][nChip-1] &= ~bitval;
            ht1632_sendData(nChip, addr, ht1632_shadowRam[addr][nChip-1]);
            break;
        case GREEN:
            // set the bit in the green plane and clear the bit in the red plane;
            ht1632_shadowRam[addr][nChip-1] |= bitval;
            ht1632_sendData(nChip, addr, ht1632_shadowRam[addr][nChip-1]);
            addr = addr + 32;
            ht1632_shadowRam[addr][nChip-1] &= ~bitval;
            ht1632_sendData(nChip, addr, ht1632_shadowRam[addr][nChip-1]);
            break;
        case RED:
            // clear the bit in green plane and set the bit in the red plane;
            ht1632_shadowRam[addr][nChip-1] &= ~bitval;
            ht1632_sendData(nChip, addr, ht1632_shadowRam[addr][nChip-1]);
            addr = addr + 32;
            ht1632_shadowRam[addr][nChip-1] |= bitval;
            ht1632_sendData(nChip, addr, ht1632_shadowRam[addr][nChip-1]);
            break;
        case ORANGE:
            // set the bit in both the green and red planes;
            ht1632_shadowRam[addr][nChip-1] |= bitval;
            ht1632_sendData(nChip, addr, ht1632_shadowRam[addr][nChip-1]);
            addr = addr + 32;
            ht1632_shadowRam[addr][nChip-1] |= bitval;
            ht1632_sendData(nChip, addr, ht1632_shadowRam[addr][nChip-1]);
            break;
    }
}


void ht1632_putChar (byte x, byte y, char c, byte color = GREEN)
{
    byte dots;
    Serial.println(c);
    if (c >= '!' && c <= '/')
        c &= 0x0F;
    else
        c = c - 32;


    for (byte row = 0; row < 8; row++) 
    {
        dots = pgm_read_byte_near(&myfont[c][row]);
        for (byte col = 0; col < 8; col++) 
        {
            if (dots & (64 >> col))
                ht1632_plot(x + col, y + row, color);
            else 
                ht1632_plot(x + col, y + row, 0);
        }
    }
}


/*
 * Draw a line between two points using the bresenham algorithm.
 * This particular bit of code is copied nearly verbatim from
 * http://www.gamedev.net/reference/articles/article1275.asp
 */
void ht1632_bresLine (byte x1, byte y1, byte x2, byte y2, byte val)
{
    char deltax = abs(x2 - x1);        // The difference between the x's
    char deltay = abs(y2 - y1);        // The difference between the y's
    char x = x1;                       // Start x off at the first pixel
    char y = y1;                       // Start y off at the first pixel
    char xinc1, xinc2, yinc1, yinc2, den, num, numadd, numpixels, curpixel;

    if (x2 >= x1) // The x-values are increasing
    {                
        xinc1 = 1;
        xinc2 = 1;
    }  
    else          // The x-values are decreasing
    {                          
        xinc1 = -1;
        xinc2 = -1;
    }

    if (y2 >= y1) // The y-values are increasing                
    {
        yinc1 = 1;
        yinc2 = 1;
    }
    else          // The y-values are decreasing                          
    {
        yinc1 = -1;
        yinc2 = -1;
    }

    if (deltax >= deltay)         // There is at least one x-value for every y-value
    {
        xinc1 = 0;                  // Don't change the x when numerator >= denominator
        yinc2 = 0;                  // Don't change the y for every iteration
        den = deltax;
        num = deltax / 2;
        numadd = deltay;
        numpixels = deltax;         // There are more x-values than y-values
    }
    else                          // There is at least one y-value for every x-value
    {
        xinc2 = 0;                  // Don't change the x for every iteration
        yinc1 = 0;                  // Don't change the y when numerator >= denominator
        den = deltay;
        num = deltay / 2;
        numadd = deltax;
        numpixels = deltay;         // There are more y-values than x-values
    }

    for (curpixel = 0; curpixel <= numpixels; curpixel++)
    {
        ht1632_plot(x, y, val);            // Draw the current pixel
        num += numadd;              // Increase the numerator by the top of the fraction
        if (num >= den)             // Check if numerator >= denominator
        {
            num -= den;               // Calculate the new numerator value
            x += xinc1;               // Change the x as appropriate
            y += yinc1;               // Change the y as appropriate
        }
        x += xinc2;                 // Change the x as appropriate
        y += yinc2;                 // Change the y as appropriate
    }
}

/*
 * Function to calculate thermistor temperature.
 * 10kOhm Thermistor in series with 10kOhm resistor.
 */
double thermister(int RawADC) {
    double Temp;
    // See http://en.wikipedia.org/wiki/Thermistor for explanation of formula
    Temp = log(((10240000/RawADC) - 10000));
    Temp = 1 / (0.001129148 + (0.000234125 * Temp) + (0.0000000876741 * Temp * Temp * Temp));
    Temp = Temp - 273.15;           // Convert Kelvin to Celcius
    return Temp;
}

/*
 * displayDecimal
 * Pass a decimal (between 0.00 and 99.99) value to this function and display it at the x and y values passed.
 */
void displayDecimal (float num, int x, int y)
{
    char str0[1],str1[1],str2[1],str3[1];
    int ten = num/10;
    int one = num;
    while(one > 10)
        one = one - 10;
    int hundredth = (num - ten*10 - one)/0.01;
    int tenth = hundredth / 10;
    while(hundredth > 10)
        hundredth = hundredth - 10;
 
    itoa(ten,str0,10);
    itoa(one,str1,10);
    itoa(tenth,str2,10);
    itoa(hundredth,str3,10);
    if(ten != 0)
    {
        ht1632_putChar(x,y,str0[0]);
        x += 6;
    }
    ht1632_putChar(x,y,str1[0]);
    x += 6;
    ht1632_putChar(x,y,'.');
    x += 6;
    ht1632_putChar(x,y,str2[0]);
    x += 6;
    ht1632_putChar(x,y,str3[0]); 
} 


/***********************************************************************
 * traditional Arduino sketch functions: setup and loop.
 ***********************************************************************/

void setup ()
{
    ht1632_setup();  
    randomSeed(analogRead(0));
    Serial.begin(9600);
    ht1632_clear();
}

void loop ()
{
    Serial.println(voltage);
    displayDecimal(voltage,0,0);
    demo_chars();
    delay(10);  
}
