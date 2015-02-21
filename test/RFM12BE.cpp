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
#include "RFM12BE.h"

 RFM12BE::RFM12BE() {

	buf = (struct Buffer *) malloc(sizeof(struct Buffer));
	struct Buffer* pingpong = (struct Buffer *) malloc(sizeof(struct Buffer));
	buf->next = pingpong;
	pingpong->next = buf;
 }

void RFM12BE::setInterface(Interface* interface) {
	hw = interface;
}

void RFM12BE::initialise(uint8_t ID, uint8_t freqBand, uint8_t networkid, uint8_t txPower, uint8_t airKbps, uint8_t lowVoltageThreshold)
{
  nodeId = ID;
  networkId = networkid;

  if(txPower > 7) {
	  txPower = 7;
  }

  hw->initialise();

  hw->SendCommand(0x0000); // intitial SPI transfer added to avoid power-up problem
  hw->SendCommand(RFM12B_CMD_SLEEP_MODE); // DC (disable clk pin), enable lbd

  // wait until RFM12B is out of power-up reset, this takes several *seconds*
  //while (digitalRead(RFM_IRQ) == 0) {
	  hw->SendCommand(RFM12B_CMD_STATUS);
  //}

  hw->SendCommand(RFM12B_CMD_CONFIG | (freqBand << 4));

  hw->SendCommand(RFM12B_CMD_FREQ_CENTRE);            // Frequency is exactly 434/868/915MHz (whatever freqBand is)
  hw->SendCommand(RFM12B_CMD_DATARATE | airKbps);     // Air transmission baud rate: 0x08= ~38.31Kbps

  hw->SendCommand(RFM12B_CMD_RXCTL | 0x0442);         // VDI,FAST,134kHz,0dBm,-91dBm
  hw->SendCommand(RFM12B_CMD_DATAFILTER | 0x00AC);    // AL,!ml,DIG,DQD4

  if (networkId != 0) {
    hw->SendCommand(RFM12B_CMD_FIFO | 0x0083);        // FIFO8,2-SYNC,!ff,DR
    hw->SendCommand(RFM12B_CMD_SYNCHRON | (uint16_t)networkId); // SYNC=2D<networkId>;
  } else {
    hw->SendCommand(RFM12B_CMD_FIFO | 0x008B);        // FIFO8,1-SYNC,!ff,DR
    hw->SendCommand(RFM12B_CMD_SYNCHRON | 0x002D);    // SYNC=2D;
  }

  hw->SendCommand(RFM12B_CMD_AFC   | 0x0083);           // @PWR,NO RSTRIC,!st,!fi,OE,EN
  hw->SendCommand(RFM12B_CMD_TXCTL | 0x00c0 | txPower); // !mp,195kHz,MAX OUT  //last byte=power level: 0=highest, 7=lowest
  hw->SendCommand(RFM12B_CMD_PLL   | 0x0077);           // OB1,OB0, LPX,!ddy,DDIT,BW0
  hw->SendCommand(RFM12B_CMD_WAKEUP);                   // wakeup time = 0
  hw->SendCommand(RFM12B_CMD_LDC_OFF);                      // LDC disabled
  hw->SendCommand(RFM12B_CMD_LBD_UCK);                  // Clock out 1MHz. Low Voltage threshold (2.2V)

  mode = MODE_IDLE;

}

// Registers an event handler
// Will replace an existing handler with the same mask at the currently registered
// position.
void RFM12BE::registerEventHandler(uint8_t mask, RadioEventHandler handler) {

    if(registeredEventHandlers < MAX_EVENT_HANDLERS) {

        for(int i=0; i<registeredEventHandlers; i++) {
            if(eventHandlers[registeredEventHandlers].mask == mask) {
                // replace the old handler
                eventHandlers[i].mask = mask;
                eventHandlers[i].handler = handler;
                return;
            }
        }

        eventHandlers[registeredEventHandlers].mask = mask;
        eventHandlers[registeredEventHandlers++].handler = handler;
    }
}

// Removes all registered event handlers
void RFM12BE::clearEventHandlers() {
    registeredEventHandlers = 0;
}

// Run any callback handlers in the order in which they were registered
void RFM12BE::dispatchEvents() {

    RegisteredEventHandler* handler;

	for(uint8_t i=0; i<registeredEventHandlers; i++) {
		handler = &eventHandlers[i];
		if(events & handler->mask) {
			if(handler->handler()) {
				events &= ~handler->mask;
			}
		}
    }
}

// Start the receiver listening for incoming packets
// Abort any in-flight transactions
// Clear any pending TX or RX events, including errors
// Do not clear LBD, POR, WAKEUP or EXTINT events
void RFM12BE::startReceive() {
	hw->beginTransaction();

	// Tx/Rx off
	hw->SendCommand(RFM12B_CMD_IDLE);

	if(mode == MODE_RX_RECV) {
		// If we're already in receive mode, reset the FIFO
		hw->SendCommand(RFM12B_CMD_FIFO_RESET);
		hw->SendCommand(RFM12B_CMD_FIFO_ENABLE);
	}

	// Clear any interrupts
	hw->SendCommand(RFM12B_CMD_STATUS);

	// Receiver on
	hw->SendCommand(RFM12B_CMD_RX_ENABLE);

	events &= ~EVENT_TXRX_ANY;
	mode = MODE_RX_RECV;
	crc = ~0;
	crc = _crc16_update(crc, networkId);
	bufIdx = 0;
	buf->buf[BUF_LEN] = 0;
	hw->endTransaction();
}

// Call this when an EVENT_RX_RECEIVE is raised to get the buffer containing the packet
// Call this during transmit when you are ready to send the current buffer and need the next buffer
uint8_t* RFM12BE::getBuffer() {
	// we'll have moved to the other buffer for the next incoming data
	// so return the last buffer
	return buf->next->buf + BUF_DATA;
}

uint8_t RFM12BE::getSenderId() {
	return buf->next->buf[BUF_FROM];
}

// Call this to start transmission
// Abort any in-flight receive
// Immediately raises an EVENT_TX_READY
uint8_t*  RFM12BE::startTransmit() {
	hw->beginTransaction();

	// Tx/Rx off
	hw->SendCommand(RFM12B_CMD_IDLE);

	if(mode == MODE_RX_RECV) {
		// If we're already in rx mode, reset the FIFO
		hw->SendCommand(RFM12B_CMD_FIFO_RESET);
		hw->SendCommand(RFM12B_CMD_FIFO_ENABLE);
	}

	// Clear any interrupts
	hw->SendCommand(RFM12B_CMD_STATUS);

	events &= ~EVENT_TXRX_ANY;
	events |= EVENT_TX_READY;
	mode = MODE_IDLE;
	hw->endTransaction();
	return buf->buf + BUF_DATA;
}

// Start tx. Enqueue the current buffer and return the next buffer ready to be filled.
// ISR will send and stop TX when done
// don't wait for any receive in-progress to finish, just abort it
uint8_t* RFM12BE::send(uint8_t to, uint8_t len) {
	buf->buf[BUF_FROM] = nodeId;
	buf->buf[BUF_TO] = to;
	buf->buf[BUF_LEN] = len;
	crc = ~0;
	bufIdx = 0;
	hw->beginTransaction();
	hw->SendCommand(RFM12B_CMD_TX_ON);
	mode = MODE_TX_SEND;
	xState = TX_PRE1;
	hw->endTransaction();
	return buf->next->buf + BUF_DATA;
}

// Enter Low Duty Cycle mode, listening for packets
// cancel any operations in progress
void RFM12BE::sleepLDC(uint16_t wakeup, uint16_t duty) {
	startReceive();
	hw->beginTransaction();
	hw->SendCommand(RFM12B_CMD_LDC_MODE);
	hw->SendCommand(RFM12B_CMD_WAKEUP | (wakeup & ~0xe000));
	hw->SendCommand(RFM12B_CMD_LDC_ON | ( (duty << 1) & ~RFM12B_CMD_LDC_ON));
	mode = MODE_IDLE;
	hw->endTransaction();
}


// Come back out of LDC mode into IDLE mode
void RFM12BE::wakeup() {
	hw->beginTransaction();
	hw->SendCommand(RFM12B_CMD_LDC_OFF);
	hw->SendCommand(RFM12B_CMD_FIFO_RESET);
	hw->SendCommand(RFM12B_CMD_FIFO_ENABLE);
	hw->SendCommand(RFM12B_CMD_IDLE);
	hw->endTransaction();
}

void RFM12BE::interruptHandler(RFM12BE* radio) {
    
    uint8_t dataByte;

    uint16_t status;
    
    radio->hw->startTransfer();
    status = radio->hw->Byte(0x00) << 8;
    status |= radio->hw->Byte(0x00);

    // Is the FIFO full? If so, read a byte slowly
    if((radio->mode == MODE_RX_RECV) && (status & INT_RGIT_FFIT)) {
    	dataByte = radio->hw->SlowByte(0x00);
    }

    radio->hw->stopTransfer();
    
    // what to do depends on the mode we're in
    // generate events from the interrupts
    radio->events |= status & (INT_POR | INT_WKUP | INT_EXT | INT_LBD);

    if(status & INT_RGIT_FFIT) {

        switch(radio->mode) {

            case MODE_IDLE:
            	// weird. Ignore it?
				// reset the FIFO just in case
				radio->hw->SendCommand(RFM12B_CMD_FIFO_RESET);
				radio->hw->SendCommand(RFM12B_CMD_FIFO_ENABLE);
				radio->SendData(dataByte);
                radio->events |= EVENT_ERR_IDLE;
                break;
                
            case MODE_RX_RECV:

            	radio->buf->buf[radio->bufIdx++] = dataByte;
                radio->crc = _crc16_update(radio->crc, dataByte);

				// Have we got all the data?
				if(radio->bufIdx >= radio->buf->buf[BUF_LEN] + BUF_PACKET_LEN || radio->bufIdx > BUF_END) {

                    if(radio->crc == 0 && (radio->buf->buf[BUF_TO] == radio->nodeId || radio->buf->buf[BUF_TO] == 0)) {
                        // Good to go
                    	radio->events |= EVENT_RX_PACKET;
                    	radio->buf = radio->buf->next;
                    } else {
                    	if(radio->crc != 0) {
                    		// CRC failed.
                    		// Raise an event in case someone cares
							radio->events |= EVENT_ERR_CRC;
						}
                    }
                	radio->bufIdx = 0;
                	radio->crc = ~0;
                	radio->crc = _crc16_update(radio->crc, radio->networkId);
                	radio->buf->buf[BUF_LEN] = 0;
					radio->hw->SendCommand(RFM12B_CMD_IDLE);
					radio->hw->SendCommand(RFM12B_CMD_RX_ENABLE);
                }

                break;
            
            case MODE_TX_SEND:
                
				switch(radio->xState++) {

					case TX_PRE1:
					case TX_PRE2:
					case TX_PRE3:
						dataByte = 0xAA;
						break;

					case TX_SYN1:
						dataByte = 0x2D;
						break;

					case TX_SYN2:
						dataByte = radio->networkId;
						radio->crc = _crc16_update(radio->crc, dataByte);
						break;

					case TX_DATA:
						dataByte = radio->buf->buf[radio->bufIdx];
						radio->crc = _crc16_update(radio->crc, radio->buf->buf[radio->bufIdx++]);

						// Keep sending data till it's all sent
						if(radio->bufIdx < radio->buf->buf[BUF_LEN] + BUF_DATA) {
							radio->xState = TX_DATA;
						}
						break;

					case TX_CRC1:
						dataByte = radio->crc;
						break;

					case TX_CRC2:
						dataByte = radio->crc >> 8;
						break;

					case TX_TAIL:
						dataByte = RFM12B_TAIL_BYTE;
						break;

					case TX_DONE:
						radio->hw->SendCommand(RFM12B_CMD_IDLE);
						// Ready for a new packet
						radio->xState = TX_IDLE;
						radio->events |= EVENT_TX_READY;
						radio->mode = MODE_IDLE;
						radio->buf = radio->buf->next;
						// fall through to send a tx byte

					default:
						// weird. send a byte to clear the RGIT flag
						dataByte = RFM12B_TAIL_BYTE;
				}

				radio->SendData(dataByte);
        }
    }
}

// Send the TX register write command with the lower byte of data
void RFM12BE::SendData(uint8_t data) {
	hw->SendCommand(RFM12B_CMD_TXREG_WRITE | data);
}

// Send the read FIFO command at high speed, then read a byte at low speed
uint8_t RFM12BE::ReadFIFO() {

	hw->startTransfer();
	hw->Byte(RFM12B_CMD_READ_FIFO >>8);
	uint8_t reply = hw->SlowByte(0x00);
	hw->stopTransfer();

	return reply;
}

// Send the status read command at high speed and return the result
uint16_t RFM12BE::ReadStatus() {

	hw->startTransfer();
	uint16_t reply = hw->Byte(0x00) << 8;
	reply |= hw->Byte(0x00);
	hw->stopTransfer();

	return reply;
}

uint16_t RFM12BE::getEvents() {
	return events;
}

int RFM12BE::getMode() {
	return mode;
}
