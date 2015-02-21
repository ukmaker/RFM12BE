/*
 * Interface.h
 *
 *  Created on: 7 Feb 2015
 *
 *      Author: Duncan McIntyre <duncan@calligram.co.uk>
 *
 *      Declares the hardware interface to an RFM12B
 *
 *      Pin mappings for INTN, SSN, SCK, MOSI, MISO
 *
 *
 */

#ifndef INTERFACE_H_
#define INTERFACE_H_

#include <Arduino.h>
#include <inttypes.h>

// SPI port pins
#if defined(__AVR_ATmega2560__) || defined(__AVR_ATmega1280__)
  #define RFM_IRQ     2
  #define SS_DDR      DDRB
  #define SS_PORT     PORTB
  #define SS_BIT      0
  #define SPI_SS      53    // PB0, pin 19
  #define SPI_MOSI    51    // PB2, pin 21
  #define SPI_MISO    50    // PB3, pin 22
  #define SPI_SCK     52    // PB1, pin 20
#elif defined(__AVR_ATmega644P__) || defined(__AVR_ATmega1284P__)
  #define RFM_IRQ     10
  #define SS_DDR      DDRB
  #define SS_PORT     PORTB
  #define SS_BIT      4
  #define SPI_SS      4
  #define SPI_MOSI    5
  #define SPI_MISO    6
  #define SPI_SCK     7
#elif defined(__AVR_ATtiny84__) || defined(__AVR_ATtiny44__)
  #define RFM_IRQ     2
  #define SS_DDR      DDRB
  #define SS_PORT     PORTB
  #define SS_BIT      1
  #define SPI_SS      1     // PB1, pin 3
  #define SPI_MISO    4     // PA6, pin 7
  #define SPI_MOSI    5     // PA5, pin 8
  #define SPI_SCK     6     // PA4, pin 9
#elif defined(__AVR_ATmega32U4__) //Arduino Leonardo, MoteinoLeo
  #define RFM_IRQ     0	    // PD0, INT0, Digital3
  #define SS_DDR      DDRB
  #define SS_PORT     PORTB
  //OLD from Jeelib: #define SS_BIT      6	    // Dig10, PB6
  #define SS_BIT      0	    // Dig17, PB0
  #define SPI_SS      17    // PB0, pin 8, Digital17
  #define SPI_MISO    14    // PB3, pin 11, Digital14
  #define SPI_MOSI    16    // PB2, pin 10, Digital16
  #define SPI_SCK     15    // PB1, pin 9, Digital15
#else
  // ATmega168, ATmega328, etc.
  #define RFM_IRQ     2
  #define SS_DDR      DDRB
  #define SS_PORT     PORTB
  #define SS_BIT      2     // for PORTB: 2 = d.10, 1 = d.9, 0 = d.8
  #define SPI_SS      10    // PB2, pin 16
  #define SPI_MOSI    11    // PB3, pin 17
  #define SPI_MISO    12    // PB4, pin 18
  #define SPI_SCK     13    // PB5, pin 19
#endif


class Interface {


public:

	void initialise();

	// Set SSN = 0
	void startTransfer();
	// Set SSN = 1
	void stopTransfer();

	// Disable interrupts ready for a sequence ofCommands
	void beginTransaction();
	// Reenable interrupts once sequence is complete
	void endTransaction();

	// Send a command to the radio at full SPI speed
	void SendCommand(uint16_t cmd);

	// Send and read a byte at full speed
	uint8_t Byte(uint8_t data);

	// Send and read a byte at 2.5MHz
	uint8_t SlowByte(uint8_t data);
};


#endif /* INTERFACE_H_ */
