/*
 * main.cpp
 *
 *  Created on: 9 Feb 2015
 *      Author: duncan
 */


#include <inttypes.h>
#include <malloc.h>

#include "Interface.h"
#include "RFM12BE.h"
#include <stdio.h>
#include "util/crc16.h"

#define RX_NODE_ID 0xaa
#define TX_NODE_ID 0x88
#define NETWORK 0xbb

void shouldRaiseCRCError();
void shouldReceivePacket();
void shouldIgnorePacketToAnotherRX_NODE_ID();
void shouldTransmitPacket();
void shouldTransmitAndReceive();

void handle(uint16_t status, uint8_t data);
void handle_t(uint16_t status, uint8_t data);
uint16_t handle_crc(uint16_t status, uint8_t data, uint16_t crc);
void expect(uint16_t events, uint8_t mode);
void expect_t(uint16_t events, uint8_t mode);
void printEvents(uint16_t events);
void printMode(uint8_t mode);

RFM12BE receiver;
RFM12BE transmitter;
Interface rxhw;
Interface txhw;

bool failed = false;

void banner(char* text) {
	printf("******************************************************\n");
	printf(text);
	printf("\n");
	printf("******************************************************\n");
}

bool eventHandler() {
	return true;
}

int main() {

	receiver.setInterface(&rxhw);
	transmitter.setInterface(&txhw);

	receiver.registerEventHandler(0xffff, eventHandler);
	transmitter.registerEventHandler(0xffff, eventHandler);

	printf("Initialise\n\n");
	receiver.initialise(RX_NODE_ID,0,NETWORK,0,0,0);
	transmitter.initialise(TX_NODE_ID,0,NETWORK,0,0,0);

	shouldRaiseCRCError();
	shouldReceivePacket();
	shouldIgnorePacketToAnotherRX_NODE_ID();

	shouldTransmitPacket();
	shouldTransmitAndReceive();

	if(failed) {
		printf("There were test failures\n");
	} else {
		printf("All tests passed\n");
	}
}

void shouldRaiseCRCError() {
	banner("shouldRaiseCRCError");
	receiver.startReceive();

	expect(0x00, MODE_RX_RECV);

	handle(INT_RGIT_FFIT, 0xbb); // group
	expect(0x00, MODE_RX_RECV);

	handle(INT_RGIT_FFIT, 0x0a); // to
	expect(0x00, MODE_RX_RECV);

	handle(INT_RGIT_FFIT, 0x00); // from
	handle(INT_RGIT_FFIT, 0x00); // len
	handle(INT_RGIT_FFIT, 0x0a); // data - looks like crc1
	handle(INT_RGIT_FFIT, 0x0a); // data - looks like crc2 - should get crc error here
	expect(EVENT_ERR_CRC, MODE_RX_RECV);
}

void shouldReceivePacket() {

	uint16_t crc = ~0;
	crc = _crc16_update(crc, NETWORK);

	banner("shouldReceivePacket");
	receiver.startReceive();

	expect(0x00, MODE_RX_RECV);

	crc = handle_crc(INT_RGIT_FFIT, RX_NODE_ID, crc); // to
	expect(0x00, MODE_RX_RECV);

	crc = handle_crc(INT_RGIT_FFIT, 0xff, crc); // from
	expect(0x00, MODE_RX_RECV);

	crc = handle_crc(INT_RGIT_FFIT, 0x04, crc); // len
	expect(0x00, MODE_RX_RECV);

	crc = handle_crc(INT_RGIT_FFIT, 0x04, crc); // d0
	expect(0x00, MODE_RX_RECV);

	crc = handle_crc(INT_RGIT_FFIT, 0x04, crc); // d1
	expect(0x00, MODE_RX_RECV);

	crc = handle_crc(INT_RGIT_FFIT, 0x04, crc); // d2
	expect(0x00, MODE_RX_RECV);

	crc = handle_crc(INT_RGIT_FFIT, 0x04, crc); // d3
	expect(0x00, MODE_RX_RECV);

	handle(INT_RGIT_FFIT, crc); // crc1
	expect(0x00, MODE_RX_RECV);

	handle(INT_RGIT_FFIT, crc >> 8); // crc2

	expect(EVENT_RX_PACKET, MODE_RX_RECV);
}

void shouldIgnorePacketToAnotherRX_NODE_ID() {

	uint16_t crc = ~0;

	banner("shouldIgnorePacketToAnotherRX_NODE_ID");
	receiver.startReceive();

	expect(0x00, MODE_RX_RECV);

	crc = handle_crc(INT_RGIT_FFIT, RX_NODE_ID+1, crc); // to
	expect(0x00, MODE_RX_RECV);

	crc = handle_crc(INT_RGIT_FFIT, 0xff, crc); // from

	crc = handle_crc(INT_RGIT_FFIT, 0x04, crc); // len

	crc = handle_crc(INT_RGIT_FFIT, 0x04, crc); // d0

	crc = handle_crc(INT_RGIT_FFIT, 0x04, crc); // d1

	crc = handle_crc(INT_RGIT_FFIT, 0x04, crc); // d2

	crc = handle_crc(INT_RGIT_FFIT, 0x04, crc); // d3

	handle(INT_RGIT_FFIT, crc); // crc1
	handle(INT_RGIT_FFIT, crc >> 8); // crc2

	expect(EVENT_ERR_CRC, MODE_RX_RECV);
}

void shouldTransmitPacket() {
	banner("shouldTransmitPacket");

	printf("********************* Start Transmit\n");
	uint8_t* buffer = transmitter.startTransmit();
	uint8_t idx = 0;
	buffer[idx++] = 'H';
	buffer[idx++] = 'e';
	buffer[idx++] = 'l';
	buffer[idx++] = 'l';
	buffer[idx++] = 'o';
	buffer[idx++] = ' ';
	buffer[idx++] = 'w';
	buffer[idx++] = 'o';
	buffer[idx++] = 'r';
	buffer[idx++] = 'l';
	buffer[idx++] = 'd';
	buffer[idx++] = '!';

	expect_t(EVENT_TX_READY, MODE_IDLE);

	printf("********************* Send\n");
	transmitter.send(RX_NODE_ID, idx);
	expect_t(0x0000, MODE_TX_SEND);

	printf("********************* Start interrupt loop\n");
	for(int i=0; i<(idx+BUF_FRAMING_LEN); i++) {

		handle_t(INT_RGIT_FFIT, 0x00);
		printf("TX %2x [%c]\n", txhw.getLastSentByte(), txhw.getLastSentByte());
		expect_t(0x0000, MODE_TX_SEND);
	}

	handle_t(INT_RGIT_FFIT, 0x00);  // send the last tail byte
	expect_t(0x0000, MODE_TX_SEND);

	handle_t(INT_RGIT_FFIT, 0x00);  // last byte sent
	expect_t(EVENT_TX_READY, MODE_IDLE);
}

void shouldTransmitAndReceive() {
	banner("shouldTransmitAndReceive");


	printf("********************* Start Transmit\n");
	uint8_t* buffer = transmitter.startTransmit();
	expect_t(EVENT_TX_READY, MODE_IDLE);

	uint8_t idx = 0;
	buffer[idx++] = 'H';
	buffer[idx++] = 'e';
	buffer[idx++] = 'l';
	buffer[idx++] = 'l';
	buffer[idx++] = 'o';
	buffer[idx++] = ' ';
	buffer[idx++] = 'w';
	buffer[idx++] = 'o';
	buffer[idx++] = 'r';
	buffer[idx++] = 'l';
	buffer[idx++] = 'd';
	buffer[idx++] = '!';


	receiver.startReceive();
	expect(0x00, MODE_RX_RECV);

	printf("********************* Preamble + sync\n");
	transmitter.send(RX_NODE_ID, idx);
	expect_t(0x0000, MODE_TX_SEND);

	/** Transmitter has to send the preamble and the sync bytes **/
	for(int i=0; i<5; i++) {

		handle_t(INT_RGIT_FFIT, 0x00);
		expect_t(0x0000, MODE_TX_SEND);
	}

	/** At this point the receiver should start seeing interrupts **/
	printf("********************* Packet\n");
	for(int i=0; i<(idx+BUF_FRAMING_LEN - BUF_PACKET_LEN - 1); i++) {

		handle_t(INT_RGIT_FFIT, 0x00);
		expect_t(0x0000, MODE_TX_SEND);
		printf("TX %2x [%c]\n", txhw.getLastSentByte(), txhw.getLastSentByte());
		handle(INT_RGIT_FFIT, txhw.getLastSentByte());
		expect(0x00, MODE_RX_RECV);
	}

	handle_t(INT_RGIT_FFIT, 0x00);  // send the last crc byte
	expect_t(0x0000, MODE_TX_SEND);

	handle(INT_RGIT_FFIT, txhw.getLastSentByte());
	expect(EVENT_RX_PACKET, MODE_RX_RECV);

	handle_t(INT_RGIT_FFIT, 0x00);  // send the tail byte
	expect_t(0x0000, MODE_TX_SEND);

	handle_t(INT_RGIT_FFIT, 0x00);  // last byte sent
	expect_t(EVENT_TX_READY, MODE_IDLE);

	buffer = receiver.getBuffer();
	if(buffer[0] != 'H') {
		printf("Failed\n");
	}
	if(buffer[1] != 'e') {
		printf("Failed\n");
	}

}
void handle(uint16_t status, uint8_t data) {
	rxhw.setMockData(status, data);
	RFM12BE::interruptHandler(&receiver);
	//printf("Events = %4x, Mode = %2x\n", receiver.getEvents(), receiver.getMode());
}

void handle_t(uint16_t status, uint8_t data) {
	txhw.setMockData(status, data);
	RFM12BE::interruptHandler(&transmitter);
	//printf("Events = %4x, Mode = %2x\n", transmitter.getEvents(), transmitter.getMode());
}

uint16_t handle_crc(uint16_t status, uint8_t data, uint16_t crc) {
	handle(status, data);
	return _crc16_update(crc, data);
}

void expect(uint16_t events, uint8_t mode) {
	uint16_t eActual = receiver.getEvents();
	uint8_t mActual = receiver.getMode();

	if((events != eActual) || (mode != mActual)) {
		printf("Expected E={"); printEvents(events);
		printf("} M="); printMode(mode);
		printf("\n");

		printf("Actual   E={"); printEvents(eActual);
		printf("} M="); printMode(mActual);
		printf("\n");

		failed = true;
	}

	receiver.dispatchEvents();
}

void expect_t(uint16_t events, uint8_t mode) {
	uint16_t eActual = transmitter.getEvents();
	uint8_t mActual = transmitter.getMode();

	if((events != eActual) || (mode != mActual)) {
		printf("Expected E={"); printEvents(events);
		printf("} M="); printMode(mode);
		printf("\n");

		printf("Actual   E={"); printEvents(eActual);
		printf("} M="); printMode(mActual);
		printf("\n");

		failed = true;
	}
	transmitter.dispatchEvents();
}

void printEvents(uint16_t events) {
	if(events & EVENT_RX_PACKET) printf("RX_PACKET ");
	if(events & EVENT_ERR_CRC) printf("ERR_CRC ");
	if(events & EVENT_TX_READY) printf("TX_READY ");
	if(events & EVENT_ERR_TX_UNDERRUN) printf("TX_UNDERRUN ");
	if(events & EVENT_ERR_IDLE) printf("ERR_IDLE ");
	if(events & EVENT_LBD) printf("LBD ");
	if(events & EVENT_POR) printf("POR ");
	if(events & EVENT_WAKEUP) printf("WAKEUP ");
	if(events & EVENT_EXTINT) printf("EXTINT ");
}

void printMode(uint8_t mode) {
	switch(mode) {
	case MODE_SLEEP: printf("SLEEP "); break;
	case MODE_IDLE: printf("IDLE "); break;
	case MODE_RX_RECV: printf("RX_RECV "); break;
	case MODE_TX_SEND: printf("TX_SEND "); break;

	}
}
