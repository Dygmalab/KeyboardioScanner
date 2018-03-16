#include <Arduino.h>
#include "KeyboardioScanner.h"
#include <Wire.h>



#define SCANNER_I2C_ADDR_BASE 0x58
#define ELEMENTS(arr)  (sizeof(arr) / sizeof((arr)[0]))

uint8_t twi_uninitialized = 1;


#ifdef __SAMD21G18A__
	#define I2C_ERROR	(1)
	#define I2C_OK		(0)
    #include "wiring_private.h" // pinPeripheral() function
    TwoWire Wire2(&sercom2, 4, 3);
	// just for reference. On Arduino Zero there are following I2C pins:
	// PIN_WIRE_SDA         (20u)
	// PIN_WIRE_SCL         (21u)
	uint8_t twi_writeTo(uint8_t addr, uint8_t* pData, size_t length, uint8_t blockingFlag, uint8_t stopFlag) {
		//in fact we always send data in blocking mode. In case of error return 1.
		Wire2.beginTransmission(addr); // transmit to device addr
		if(!Wire2.write(pData, length)) return I2C_ERROR; // sends data
		if(!Wire2.endTransmission(stopFlag)) return I2C_ERROR;// stop transmitting
		return I2C_OK;
	}
	
	uint8_t twi_readFrom(uint8_t addr, uint8_t* pData, size_t length, uint8_t stopFlag) {
		uint8_t counter = 0;
		uint32_t timeout;
		if(!Wire2.requestFrom(addr, length, stopFlag)){
			//in case slave is not responding - return 0 (0 length of received data).
			return 0;
		}
		while(Wire2.available() && (length > 0))    // slave may send less than requested
		{ 
			// receive a byte in blocking mode
			*pData = Wire2.read(); 
			pData++;
			length--;
			counter++;
		}
		return counter;
	}
	
	void twi_disable(void)
	{
		Wire2.end();
	}
	void twi_init()
	{
        Wire2.begin(); // fails here
        // Assign pins 4 & 3 to SERCOM functionality
        // pa08 is sda, pa09 is clk
        pinPeripheral(4, PIO_SERCOM_ALT);
        pinPeripheral(3, PIO_SERCOM_ALT);
        Wire2.setClock(400000);
	}
#endif

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

KeyboardioScanner::~KeyboardioScanner() {}

KeyboardioScanner::KeyboardioScanner(byte setAd01) {
  ad01 = setAd01;
  addr = SCANNER_I2C_ADDR_BASE | ad01;
  // keyReady will be true after a read when there's another key event
  // already waiting for us
  keyReady = false;
  
/*  if (twi_uninitialized--) {
    twi_init();
  }
 */ 
}

// Returns the relative controller addresss. The expected range is 0-3
uint8_t KeyboardioScanner::controllerAddress() {
  return ad01;
}

// Sets the keyscan interval. We currently do three reads.
// before declaring a key event debounced.
//
// Takes an integer value representing a counter.
//
// 0 - 0.1-0.25ms
// 1 - 0.125ms
// 10 - 0.35ms
// 25 - 0.8ms
// 50 - 1.6ms
// 100 - 3.15ms
//
// You should think of this as the _minimum_ keyscan interval.
// LED updates can cause a bit of jitter.
//
// returns the Wire.endTransmission code (0 = success)
// https://www.arduino.cc/en/Reference/WireEndTransmission
byte KeyboardioScanner::setKeyscanInterval(byte delay) {
  uint8_t data[] = {TWI_CMD_KEYSCAN_INTERVAL, delay};
  uint8_t result = twi_writeTo(addr, data, ELEMENTS(data), 1, 0);
  return result;
}




// returns -1 on error, otherwise returns the scanner version integer
int KeyboardioScanner::readVersion() {
  return readRegister(TWI_CMD_VERSION);
}

// returns -1 on error, otherwise returns the scanner keyscan interval
int KeyboardioScanner::readKeyscanInterval() {
  return readRegister(TWI_CMD_KEYSCAN_INTERVAL);
}


// returns -1 on error, otherwise returns the LED SPI Frequncy
int KeyboardioScanner::readLEDSPIFrequency() {
  return readRegister(TWI_CMD_LED_SPI_FREQUENCY);
}

// Set the LED SPI Frequency. See wire-protocol-constants.h for
// values.
//
// returns the Wire.endTransmission code (0 = success)
// https://www.arduino.cc/en/Reference/WireEndTransmission
byte KeyboardioScanner::setLEDSPIFrequency(byte frequency) {
  uint8_t data[] = {TWI_CMD_LED_SPI_FREQUENCY, frequency};
  uint8_t result = twi_writeTo(addr, data, ELEMENTS(data), 1, 0);
  return result;
}



int KeyboardioScanner::readRegister(uint8_t cmd) {

  byte return_value = 0;

  uint8_t data[] = {cmd};
  uint8_t result = twi_writeTo(addr, data, ELEMENTS(data), 1, 0);

  delayMicroseconds(15); // We may be able to drop this in the future
  // but will need to verify with correctly
  // sized pull-ups on both the left and right
  // hands' i2c SDA and SCL lines

  uint8_t rxBuffer[1];

  // perform blocking read into buffer
  uint8_t read = twi_readFrom(addr, rxBuffer, ELEMENTS(rxBuffer), true);
  if (read > 0) {
    return rxBuffer[0];
  } else {
    return -1;
  }

}


// returns the raw key code from the controller, or -1 on failure.
// returns true of a key is ready to be read
bool KeyboardioScanner::moreKeysWaiting() {
  return keyReady;
}

// gives information on the key that was just pressed or released.
bool KeyboardioScanner::readKeys() {

  uint8_t rxBuffer[6];

  // perform blocking read into buffer
  uint8_t read = twi_readFrom(addr, rxBuffer, ELEMENTS(rxBuffer), true);
  if (rxBuffer[0] == TWI_REPLY_KEYDATA) {
    keyData.rows[0] = rxBuffer[1];
    keyData.rows[1] = rxBuffer[2];
    keyData.rows[2] = rxBuffer[3];
    keyData.rows[3] = rxBuffer[4];
    keyData.rows[4] = rxBuffer[5];
    return true;
  } else {
    return false;
  }
}

keydata_t KeyboardioScanner::getKeyData() {
  return keyData;
}

void KeyboardioScanner::sendLEDData() {
  sendLEDBank(nextLEDBank++);
  if (nextLEDBank == LED_BANKS) {
    nextLEDBank = 0;
  }
}

void KeyboardioScanner::sendLEDBank(byte bank) {
  uint8_t data[LED_BYTES_PER_BANK + 1];
  data[0]  = TWI_CMD_LED_BASE + bank;
  for (uint8_t i = 0 ; i < LED_BYTES_PER_BANK; i++) {
    data[i + 1] = pgm_read_byte(&gamma8[ledData.bytes[bank][i]]);
  }
  uint8_t result = twi_writeTo(addr, data, ELEMENTS(data), 1, 0);
  if( result == 4 )
  {
    twi_disable();
    twi_init();
  }
}



void KeyboardioScanner::setAllLEDsTo(cRGB color) {
  uint8_t data[] = {TWI_CMD_LED_SET_ALL_TO,
                    pgm_read_byte(&gamma8[color.r]),
                    pgm_read_byte(&gamma8[color.g]),
                    pgm_read_byte(&gamma8[color.b])
                   };
  uint8_t result = twi_writeTo(addr, data, ELEMENTS(data), 1, 0);
}

void KeyboardioScanner::setOneLEDTo(byte led, cRGB color) {
  uint8_t data[] = {TWI_CMD_LED_SET_ONE_TO,
                    led,
                    pgm_read_byte(&gamma8[color.r]),
                    pgm_read_byte(&gamma8[color.g]),
                    pgm_read_byte(&gamma8[color.b])
                   };
  uint8_t result = twi_writeTo(addr, data, ELEMENTS(data), 1, 0);

}


