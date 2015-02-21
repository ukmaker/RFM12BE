#include "Interface.h"
#include <stdio.h>
#include <inttypes.h>

void Interface::setMockData(uint16_t status, uint8_t data) {
	byte1 = status >> 8;
	byte2 = status;
	byte3 = data;
}

uint8_t Interface::getLastSentByte() {
	return lastSentByte;
}

// Set up the SPI interface
void Interface::initialise() {
	inTx = false;
	printf("Initialise\n");
}

void Interface::startTransfer() {
	printf("SSN=0 ");
}

void Interface::stopTransfer() {
	printf("SSN=1 ");
	if(!inTx) printf("\n");
}

void Interface::beginTransaction() {
	printf("INT OFF ");
	inTx = true;
}

void Interface::endTransaction() {
	printf("INT ON\n");
	inTx = false;
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
	  printf("Byte %2x -> %2x ", data, byte1);
	  uint8_t result = byte1;
	  byte1 = byte2;
	  byte2 = byte3;
	  lastSentByte = data;
	  return result;
}

// Send and read a byte at 2.5MHz
// Callers must manipulate the SSN pin
uint8_t Interface::SlowByte(uint8_t data) {
	printf("Slow");
	return Byte(data);
}

