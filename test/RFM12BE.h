/**
 * http://opensource.org/licenses/mit-license.php
 *
 * 2015-02-21 (C) Duncan McIntyre <duncan@calligram.co.uk>
 *
 * Inspired by:
 *
 *    The RFM12B driver from felix@lowpowerlab.com
 *    The RFM12 driver from jeelabs.com (2009-02-09 <jc@wippler.nl>)
 *
 **/
#ifndef RFM12BE_H
#define RFM12BE_H

#include <Arduino.h>
#include <util/crc16.h>
#include <inttypes.h>
#include "Interface.h"

// You should normally define this in your application
// This is the size of the data buffer
// The overall memory size used for buffers will be a little more then double this, since two
// buffers are allocated
#ifndef BUFSIZE
#define BUFSIZE 64
#endif

// To conserve memory, define this constant to declare the maximum number of event handlers you will register
#ifndef MAX_EVENT_HANDLERS
#define MAX_EVENT_HANDLERS 2
#endif

// RFM12B Interrupt bits
#define INT_RGIT_FFIT  0x8000
#define INT_POR        0x4000
#define INT_RGUR_FFOV  0x2000
#define INT_WKUP       0x1000
#define INT_EXT        0x0800
#define INT_LBD        0x0400

// RFM12B status bits
#define STAT_FFEM      0x0200
#define STAT_RSSI_ATS  0x0100
#define STAT_DQD       0x0080
#define STAT_CRL       0x0040
#define STAT_ATGL      0x0020

#define STAT_OFFS6     0x0010
#define STAT_OFFS3     0x0008
#define STAT_OFFS2     0x0004
#define STAT_OFFS1     0x0002
#define STAT_OFFS0     0x0001

// Constants defining RFM12B commands
// Internal data register, FIFO mode, 12pF
#define RFM12B_CMD_CONFIG      0x80C7

#define RFM12B_CMD_POWER       0x8200
#define RFM12B_CMD_IDLE        0x820C
#define RFM12B_CMD_RX_ENABLE   0x828C
#define RFM12B_CMD_SLEEP_MODE  0x8204
#define RFM12B_CMD_TX_ON       0x823C
#define RFM12B_CMD_LDC_MODE    0x8202

#define RFM12B_CMD_FREQ_CENTRE 0xA640
#define RFM12B_CMD_DATARATE    0xC600

#define RFM12B_CMD_RXCTL       0x9000
#define RFM12B_CMD_DATAFILTER  0xC228

#define RFM12B_CMD_FIFO        0xCA00
#define RFM12B_CMD_FIFO_RESET  0xCA81
#define RFM12B_CMD_FIFO_ENABLE 0xCA83

#define RFM12B_CMD_READ_FIFO   0xB000
#define RFM12B_CMD_SYNCHRON    0xCE00
#define RFM12B_CMD_AFC         0xC400
#define RFM12B_CMD_TXCTL       0x9800
#define RFM12B_CMD_PLL         0xCC12
#define RFM12B_CMD_TXREG_WRITE 0xB800

#define RFM12B_CMD_WAKEUP      0xE000
#define RFM12B_CMD_LDC_OFF     0xC800
#define RFM12B_CMD_LDC_ON      0xC801
#define RFM12B_CMD_LBD_UCK     0xC000
#define RFM12B_CMD_STATUS      0x0000

#define RFM12B_TAIL_BYTE 0xAA

//frequency bands
#define RF12_315MHZ     0
#define RF12_433MHZ     1
#define RF12_868MHZ     2
#define RF12_915MHZ     3

/**
 *
 * States:
 *   SLEEP   -> RX_RECV or IDLE
 *   IDLE    -> RX_RECV, TX_SEND or SLEEP
 *   TX_SEND -> IDLE
 */

// Go to sleep (LDC)
#define MODE_SLEEP 0

// Sit around powered up but with RX and TX turned off
#define MODE_IDLE 1

// Receive packets
// When a packet is received, raise EVENT_RX_PACKET
#define MODE_RX_RECV 2

// Transmit a packet
// When done, raise EVENT_TX_READY and return to IDLE
#define MODE_TX_SEND 3

// Transmit sub-states
#define TX_IDLE 0
#define TX_PRE1 1
#define TX_PRE2 2
#define TX_PRE3 3
#define TX_SYN1 4
#define TX_SYN2 5
#define TX_DATA 6
#define TX_CRC1 7
#define TX_CRC2 8
#define TX_TAIL 9
#define TX_DONE 10

// Receive sub-states
#define RX_FROM 1
#define RX_TO   2
#define RX_LEN  3
#define RX_DATA 4
#define RX_CRC1 5
#define RX_CRC2 6
#define RX_DONE 7

////
// Definitions of the events which can be raised by the driver
////

// Raised on first and subsequent packets
#define EVENT_RX_PACKET  0x0001

// Raised when a CRC error occurs during packet reception
#define EVENT_ERR_CRC         0x0002

// A mask for any RX event
#define EVENT_RX_ANY 0x0003

// Raised when a packet has been transmitted
#define EVENT_TX_READY        0x0004
#define EVENT_ERR_TX_UNDERRUN 0x0008

#define EVENT_TX_ANY          0x000C

#define EVENT_TXRX_ANY        0x000F

// Raised when an unexpected interrupt arrives during idle
#define EVENT_ERR_IDLE        0x0010

// Other events map directly to their interrupt bits
#define EVENT_LBD        0x0400
#define EVENT_POR        0x4000
#define EVENT_WAKEUP     0x1000
#define EVENT_EXTINT     0x0800

// Mask of all the error bits
#define EVENT_ERROR   (EVENT_ERR_CRC|EVENT_ERR_RX|EVENT_ERR_TX_UNDERRUN|EVENT_ERR_TX_STATE|EVENT_ERR_IDLE)

// Event handlers should return true if the want the event flag(s) to be cleared
typedef bool (*RadioEventHandler)(void);

typedef struct {
	uint8_t mask;
    RadioEventHandler handler;
} RegisteredEventHandler ;

// Buffer structure
// 0             toAddr
// 1             fromAddr
// 2             len
// 3..3+len      data
// 4+len - 5+len crc
#define BUF_TO 0
#define BUF_FROM 1
#define BUF_LEN 2
#define BUF_DATA 3
#define BUF_END (BUFSIZE + 4)
// Total frame size is preamble(3)+sync(2)+to+from+len+crc(2)
#define BUF_FRAMING_LEN 10
// Length of a packet excluding framing and data
#define BUF_PACKET_LEN 5

typedef struct Buffer {

    uint8_t buf[BUFSIZE+BUF_PACKET_LEN];

    Buffer *next;

} Buffer;



class RFM12BE {

	 uint8_t nodeId;
	 uint8_t networkId;

	 volatile int mode;

	// The RX or TX sub-state
	 volatile int xState;

	 volatile uint8_t bufIdx;
	 volatile uint16_t crc;

	 volatile uint8_t events;
	 volatile uint8_t err;

	 struct Buffer *buf;

	 RegisteredEventHandler eventHandlers[MAX_EVENT_HANDLERS];
	 uint8_t registeredEventHandlers;

	 Interface* hw;


public:

	RFM12BE();
	void setInterface(Interface* interface);

	// initialise. Start ISR
	void initialise(uint8_t ID, uint8_t freqBand, uint8_t networkid, uint8_t txPower, uint8_t airKbps, uint8_t lowVoltageThreshold);

	// Start the receiver listening for incoming packets
	void startReceive();

	uint8_t* getBuffer();

	uint8_t getSenderId();

	// Call this before transmission
	// Cancels any current transaction and puts the radio into IDLE mode
	// Returns the tx buffer
	// Immediately raises an EVENT_TX_READY
	uint8_t* startTransmit();

	// Start tx. Enqueue the current buffer and return the next buffer ready to be filled.
	// ISR will send and stop TX when done
	// don't wait for any receive in-progress to finish, just abort it
	uint8_t* send(uint8_t to, uint8_t len);

	// Enter Low Duty Cycle mode, listening for packets
	// cancel any operations in progress
	void sleepLDC(uint16_t wakeup, uint16_t duty);
	void wakeup();


	// Register an event handler to handle the events specified by the mask
	// Note that you can only register up to MAX_EVENT_HANDLERS
	void registerEventHandler(uint8_t mask, RadioEventHandler handler);

	// Removes all registered event handlers
	void clearEventHandlers();

	// Run any callback handlers *in the order in which they were registered*
	void dispatchEvents();

	static void interruptHandler(RFM12BE* radio);


	// Write a byte to the tx register at full speed
	 void SendData(uint8_t data);

	// Send the ReadFIFO command at full SPI speed, switch to 2.5MHz to read the byte back
	 uint8_t ReadFIFO();

	// Read the status bits at full speed
	 uint16_t ReadStatus();

	 uint16_t getEvents();

	 int getMode();
};

#endif
