Interface:
	mkdir -p build && cd build && \
	g++ -O0 -I ../test/include -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"Interface.d" -MT"Interface.d" -o "Interface.o" "../test/Interface.cpp"

RFM12BE:
	mkdir -p build && cd build && \
	g++ -O0 -I ../test/include -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"RFM12BE.d" -MT"RFM12BE.d" -o "RFM12BE.o" "../src/RFM12BE.cpp"

crc:
	mkdir -p build && cd build && \
	g++ -O0 -I ../test/include -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"crc16.d" -MT"crc16.d" -o "crc16.o" "../test/crc16.cpp"

main: Interface RFM12BE crc
	mkdir -p build && cd build && \
	cp ../src/RFM* ../test && \
	g++ -O0 -I ../test/include -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"main.d" -MT"main.d" -o "main.o" "../test/main.cpp" && \
	g++  -o "Tests"  Interface.o RFM12BE.o crc16.o main.o

test: main
	build/Tests