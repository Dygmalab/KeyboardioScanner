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

