#include <Arduino.h>
#include "KeyboardioScanner.h"
#include <Wire.h>



#define SCANNER_I2C_ADDR_BASE 0x58
#define ELEMENTS(arr)  (sizeof(arr) / sizeof((arr)[0]))

uint8_t twi_uninitialized = 1;


#ifdef __SAMD21G18A__
	#define I2C_ERROR	(1)
	#define I2C_OK		(0)
	uint8_t twi_writeTo(uint8_t addr, uint8_t* pData, size_t length, uint8_t blockingFlag, uint8_t stopFlag) {
		//in fact we always send data in blocking mode. In case of error return 1.
		Wire.beginTransmission(addr); // transmit to device addr
		if(!Wire.write(pData, length)) return I2C_ERROR; // sends data
		if(Wire.endTransmission(stopFlag) != 0) return I2C_ERROR;// stop transmitting
		return I2C_OK;
	}
	
	uint8_t twi_readFrom(uint8_t addr, uint8_t* pData, size_t length, uint8_t stopFlag) {
		uint8_t counter = 0;
		uint32_t timeout;
		if(!Wire.requestFrom(addr, length, stopFlag)){
			//in case slave is not responding - return 0 (0 length of received data).
			return 0;
		}
		while(Wire.available() && (length > 0))    // slave may send less than requested
		{ 
			// receive a byte in blocking mode
			*pData = Wire.read(); 
			pData++;
			length--;
			counter++;
		}
		return counter;
	}
	
	void twi_disable(void)
	{
		Wire.end();
	}
	void twi_init()
	{
        Wire.begin();
        Wire.setClock(400000);
	}
#endif


KeyboardioScanner::~KeyboardioScanner() {}

KeyboardioScanner::KeyboardioScanner(byte setAd01) {
  ad01 = setAd01;
  addr = SCANNER_I2C_ADDR_BASE | ad01;
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
  uint8_t result = twi_writeTo(addr, data, ELEMENTS(data), 1, 1);
  return result;
}




// returns -1 on error, otherwise returns the scanner version integer
int KeyboardioScanner::readVersion() {
  return readRegister(TWI_CMD_VERSION);
}


// returns -1 on error, otherwise returns the sled version integer
int KeyboardioScanner::readSLEDVersion() {
  return readRegister(TWI_CMD_SLED_STATUS);
}
// returns -1 on error, otherwise returns the sled current settings
int KeyboardioScanner::readSLEDCurrent() {
  return readRegister(TWI_CMD_SLED_CURRENT);
}
byte KeyboardioScanner::setSLEDCurrent(byte current) {
  uint8_t data[] = {TWI_CMD_SLED_CURRENT, current};
  uint8_t result = twi_writeTo(addr, data, ELEMENTS(data), 1, 1);
  return result;
}

// returns -1 on error, otherwise returns the scanner keyscan interval
int KeyboardioScanner::readKeyscanInterval() {
  return readRegister(TWI_CMD_KEYSCAN_INTERVAL);
}

// returns -1 on error, otherwise returns the ANSI/ISO setting
int KeyboardioScanner::readANSI_ISO() {
  return readRegister(TWI_CMD_ANSI_ISO);
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
  uint8_t result = twi_writeTo(addr, data, ELEMENTS(data), 1, 1);
  return result;
}

// returns -1 on error, otherwise returns the value of the hall sensor integer
int KeyboardioScanner::readJoint() {
  byte return_value = 0;

  uint8_t data[] = {TWI_CMD_JOINED};
  uint8_t result = twi_writeTo(addr, data, ELEMENTS(data), 1, 1); // needed to change stopFlag to 1 to get responses from the tiny
  if(result == I2C_ERROR)
     return -1;

  // needs to be long enough for the slave to respond
  delayMicroseconds(40); 

  uint8_t rxBuffer[2];

  // perform blocking read into buffer
  uint8_t read = twi_readFrom(addr, rxBuffer, ELEMENTS(rxBuffer), true);
  if (read == 2) {
    return rxBuffer[0] + (rxBuffer[1] << 8);
  } else {
    return -1;
  }
}


int KeyboardioScanner::readRegister(uint8_t cmd) {

  byte return_value = 0;

  uint8_t data[] = {cmd};
  uint8_t result = twi_writeTo(addr, data, ELEMENTS(data), 1, 1); // needed to change stopFlag to 1 to get responses from the tiny
  if(result == I2C_ERROR)
     return -1;

  // needs to be long enough for the slave to respond
  delayMicroseconds(40); 

  uint8_t rxBuffer[1];

  // perform blocking read into buffer
  uint8_t read = twi_readFrom(addr, rxBuffer, ELEMENTS(rxBuffer), true);
  if (read > 0) {
    return rxBuffer[0];
  } else {
    return -1;
  }

}


// gives information on the key that was just pressed or released.
bool KeyboardioScanner::readKeys() {
  uint8_t rxBuffer[6] = {0,0,0,0,0,0};

  // perform blocking read into buffer
  uint8_t result = twi_readFrom(addr, rxBuffer, ELEMENTS(rxBuffer), true);
  // if result isn't 6? this can happens if slave nacks while trying to read
  KeyboardioScanner::online = result ? true : false;

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

  uint8_t data[LED_BYTES_PER_BANK + 1]; // + 1 for the update LED command itself
  data[0]  = TWI_CMD_LED_BASE + bank;
  for (uint8_t i = 0 ; i < LED_BYTES_PER_BANK; i++) {
    data[i + 1] = pgm_read_byte(&gamma8[ledData.bytes[bank][i]]);
  }
  uint8_t result = twi_writeTo(addr, data, ELEMENTS(data), 1, 1);
  /*
  if( result == 1 )
  {
    twi_disable();
    twi_init();
  }
  */
}



void KeyboardioScanner::setAllLEDsTo(cRGB color) {
  uint8_t data[] = {TWI_CMD_LED_SET_ALL_TO,
                    pgm_read_byte(&gamma8[color.r]),
                    pgm_read_byte(&gamma8[color.g]),
                    pgm_read_byte(&gamma8[color.b])
                   };
  uint8_t result = twi_writeTo(addr, data, ELEMENTS(data), 1, 1);
}

void KeyboardioScanner::setOneLEDTo(byte led, cRGB color) {
  uint8_t data[] = {TWI_CMD_LED_SET_ONE_TO,
                    led,
                    pgm_read_byte(&gamma8[color.r]),
                    pgm_read_byte(&gamma8[color.g]),
                    pgm_read_byte(&gamma8[color.b])
                   };
  uint8_t result = twi_writeTo(addr, data, ELEMENTS(data), 1, 1);

}


