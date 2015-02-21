#ifndef INTERFACE_H
#define INTERFACE_H

#include <inttypes.h>

// Grotesque hack to declare something which looks like Interface
// There's probably a correct way to do this without having to write
// virtual myfunc()=0;
// and having to deal with vtable inefficiency, but I can't be bothered
// to figure it out
class Interface {
         uint8_t byte1;
         uint8_t byte2;
         uint8_t byte3;
         uint8_t lastSentByte;

         bool inTx;

public:
	void setMockData(uint16_t, uint8_t);
 	uint8_t getLastSentByte();

	// Declare that this class implements the methods from Interface
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

#endif