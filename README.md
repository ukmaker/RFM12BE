A non-blocking driver for the RFM12B radio module. 

Communication with the main application happens through event handlers.

The hardware interface to an Arduino is isolated in the Interface class. One day there may be
a different implementation for use with e.g. a dsPIC. But if you need something different it should
be easy enough to write.

The make target 'test' builds using the platform compiler and uses a mock Interface to
run some unit tests against the RFM12BE implementation.

To use in an Arduino project, create a folder
  $ARDUINO_INSTALL_DIR/libraries/RFM12BE

where $ARDUINO_INSTALL_DIR is wherever the arduino software on your platform normally resides.
E.g. on a Linux platform this is ~/Arduino

Then copy the source files into that directory:

RFM12BE=$ARDUINO_INSTALL_DIR/libraries/RFM12BE

cp src/RFM12BE.* $RFM12BE
cp src/Interface.* $RFM12BE

!! Cruft alert!!

I am not a C++ programmer. If you like the code, great. If not, constructive comments and pull requests
are welcome. 

