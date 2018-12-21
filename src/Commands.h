#define ID_LEN 3
#define CMD_LEN 8
#define DATA_LEN 8
#define ADDR_LEN 7
#define PANEL_HEADER_BITS (ID_LEN + ADDR_LEN)

#define CS_NONE 0x00 /* None of ht1632c selected */
#define CS_ALL 0xff  /* All of ht1632c selected */

/*
 * commands written to the chip consist of a 3 bit "ID", followed by
 * either 9 bits of "Command code" or 7 bits of address + 4 bits of data.
 */
enum COMMAND_CODE {
  SYSDIS = 0x00, /* CMD= 0000-0000-x Turn off oscil */
  SYSON = 0x01,  /* CMD= 0000-0001-x Enable system oscil */
  LEDOFF = 0x02, /* CMD= 0000-0010-x Turn off LED duty cycle generator */
  LEDON = 0x03,  /* CMD= 0000-0011-x Turn on LED duty cycle generator */
  BLOFF = 0x08,  /* CMD= 0000-1000-x Blink OFF */
  BLON = 0x09,   /* CMD= 0000-1001-x Blink ON */
  SLVMD = 0x10,  /* CMD= 0001-00xx-x Slave Mode */
  MSTMD = 0x14,  /* CMD= 0001-01xx-x Master Mode */
  RCCLK = 0x18,  /* CMD= 0001-10xx-x Use on-chip clock */
  EXTCLK = 0x1C, /* CMD= 0001-11xx-x Use external clock */
  COMS00 = 0x20, /* CMD= 0010-ABxx-x commons options */
  COMS01 = 0x24, /* CMD= 0010-ABxx-x commons options */
  COMS10 = 0x28, /* CMD= 0010-ABxx-x commons options */
  COMS11 = 0x2C, /* CMD= 0010-ABxx-x commons options */
  PWM = 0xA0,    /* CMD= 101x-PPPP-x PWM duty cycle */
};

enum ID_CODE {
  CMD = 4,
  WR = 5,
  RD = 6,
};
