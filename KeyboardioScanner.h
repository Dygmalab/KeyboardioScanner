#pragma once

#include <Arduino.h>
#include "wire-protocol-constants.h"

#ifdef __SAMD21G18A__
	void twi_init(); // allow to be called by the Raise hardware setup
#endif

struct cRGB {
  uint8_t r;
  uint8_t g;
  uint8_t b;
};

const uint8_t PROGMEM gamma8[] = {
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,
  1,  1,  1,  1,  1,  1,  1,  1,  1,  2,  2,  2,  2,  2,  2,  2,
  2,  3,  3,  3,  3,  3,  3,  3,  4,  4,  4,  4,  4,  5,  5,  5,
  5,  6,  6,  6,  6,  7,  7,  7,  7,  8,  8,  8,  9,  9,  9, 10,
  10, 10, 11, 11, 11, 12, 12, 13, 13, 13, 14, 14, 15, 15, 16, 16,
  17, 17, 18, 18, 19, 19, 20, 20, 21, 21, 22, 22, 23, 24, 24, 25,
  25, 26, 27, 27, 28, 29, 29, 30, 31, 32, 32, 33, 34, 35, 35, 36,
  37, 38, 39, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 50,
  51, 52, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 66, 67, 68,
  69, 70, 72, 73, 74, 75, 77, 78, 79, 81, 82, 83, 85, 86, 87, 89,
  90, 92, 93, 95, 96, 98, 99, 101, 102, 104, 105, 107, 109, 110, 112, 114,
  115, 117, 119, 120, 122, 124, 126, 127, 129, 131, 133, 135, 137, 138, 140, 142,
  144, 146, 148, 150, 152, 154, 156, 158, 160, 162, 164, 167, 169, 171, 173, 175,
  177, 180, 182, 184, 186, 189, 191, 193, 196, 198, 200, 203, 205, 208, 210, 213,
  215, 218, 220, 223, 225, 228, 231, 233, 236, 239, 241, 244, 247, 249, 252, 255
};

#define LED_BANKS 9

#define LEDS_PER_HAND 36
#define LEDS_PER_UNDERGLOW 35
#define LED_BYTES_PER_BANK 24 
//sizeof(cRGB)  * (LEDS_PER_HAND+LEDS_PER_UNDERGLOW)/LED_BANKS

typedef union {
  cRGB leds[LEDS_PER_HAND+LEDS_PER_UNDERGLOW];
  byte bytes[LED_BANKS][LED_BYTES_PER_BANK];
} LEDData_t;


// Same datastructure as on the other side
typedef union {
  struct {
    uint8_t row: 2,
            col: 3,
            keyState: 1,
            keyEventsWaiting: 1,
            eventReported: 1;
  };
  uint8_t val;
} key_t_raise;

/*
typedef union {
  uint8_t rows[4];
  uint32_t all;
} keydata_t;
*/
typedef union {
  uint8_t rows[5];
  uint64_t all;
} keydata_t;

// config options

// used to configure interrupts, configuration for a particular controller
class KeyboardioScanner {
 public:
  KeyboardioScanner(byte setAd01);
  ~KeyboardioScanner();

  int readVersion();

  byte setKeyscanInterval(byte delay);
  int readKeyscanInterval();

  byte setLEDSPIFrequency(byte frequency);
  int readLEDSPIFrequency();

  bool moreKeysWaiting();
  void sendLEDData();
  void setOneLEDTo(byte led, cRGB color);
  void setAllLEDsTo(cRGB color);
  keydata_t getKeyData();
  bool readKeys();
  LEDData_t ledData;
  uint8_t controllerAddress();

 private:
  int addr;
  int ad01;
  keydata_t keyData;
  bool keyReady = false;
  byte nextLEDBank = 0;
  void sendLEDBank(byte bank);
  int readRegister(uint8_t cmd);
};

