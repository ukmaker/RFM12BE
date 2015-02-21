#include "Interface.h"


// Set up the SPI interface
void Interface::initialise() {

	bitSet(SS_PORT, SS_BIT);
	bitSet(SS_DDR, SS_BIT);

	pinMode(SPI_MOSI, OUTPUT);
	pinMode(SPI_MISO, INPUT);
	pinMode(SPI_SCK, OUTPUT);

	#ifdef SPCR
		SPCR = _BV(SPE) | _BV(MSTR);
		#if F_CPU > 10000000
		  // use clk/2 (2x 1/4th) for sending (and clk/8 for recv, see XFERSlow)
		  SPSR |= _BV(SPI2X);
		#endif
	#else
		// ATtiny
		USICR = bit(USIWM0);
	#endif

	pinMode(RFM_IRQ, INPUT);
	digitalWrite(RFM_IRQ, 1); // pull-up
}

void Interface::startTransfer() {
	bitClear(SS_PORT, SS_BIT);
}

void Interface::stopTransfer() {
	bitSet(SS_PORT, SS_BIT);
}

void Interface::beginTransaction() {
	bitClear(EIMSK, INT0);
}

void Interface::endTransaction() {
	bitSet(EIMSK, INT0);
}

// Write 16 bits of generic data to the radio
void Interface::SendCommand(uint16_t cmd) {
	startTransfer();
	Byte(cmd >> 8);
	Byte(cmd);
	stopTransfer();
}

// Send and read a byte at full speed
// Callers must manipulate the SSN pin
uint8_t Interface::Byte(uint8_t data) {
	  SPDR = data;
	  while (!(SPSR & _BV(SPIF)));
	  return SPDR;
}

// Send and read a byte at 2.5MHz
// Callers must manipulate the SSN pin
uint8_t Interface::SlowByte(uint8_t data) {
	// slow down to under 2.5 MHz
	// for a 16MHZ part this will be 2MHz, for a 20MHz part 2.5MHz
	#if F_CPU > 10000000
	  bitSet(SPCR, SPR0);
	#endif
	  uint8_t reply = Byte(data);
	#if F_CPU > 10000000
	  bitClear(SPCR, SPR0);
	#endif
	  return reply;
}
