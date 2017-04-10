CC=gcc
CFLAGS = -g 
# uncomment this for SunOS
# LIBS = -lsocket -lnsl

all: udp-send udp-recv udp-split-send

udp-send: udp-send.o 
	$(CC) -o udp-send udp-send.o $(LIBS)

udp-recv: udp-recv.o 
	$(CC) -o udp-recv udp-recv.o $(LIBS)
	
udp-split-send: udp-split-send.o
	$(CC) -o udp-split-send udp-split-send.o $(LIBS)

udp-send.o: udp-send.c port.h

udp-recv.o: udp-recv.c port.h

udp-split-send.o: udp-split-send.c port.h

clean:
	rm -f udp-send udp-recv udp-send.o udp-recv.o udp-split-send.o
