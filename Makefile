GCCFLAGS=-g -Os -Wall -mmcu=atmega168 
LINKFLAGS=-Wl,-u,vfprintf -lprintf_flt -Wl,-u,vfscanf -lscanf_flt -lm
AVRDUDEFLAGS=-c avrisp -p m168 -b 19200 -P COM2
LINKOBJECTS=

all: identitydisc-upload

identitydisc.hex: identitydisc.c
	make -C ../testdir
	avr-gcc ${GCCFLAGS} ${LINKFLAGS} -o identitydisc.o identitydisc.c ${LINKOBJECTS}
	avr-objcopy -j .text -O ihex identitydisc.o identitydisc.hex
	
identitydisc.ass: identitydisc.hex
	avr-objdump -S -d identitydisc.o > identitydisc.ass
	
identitydisc-upload: identitydisc.hex
	avrdude ${AVRDUDEFLAGS} -U flash:w:identitydisc.hex:a
