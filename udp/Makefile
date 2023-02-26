CC=gcc
CFLAGS=-g -Wall

all: recv_udp send_udp fancy_recv_udp

recv_udp: recv_udp.o
	$(CC) -o recv_udp recv_udp.o

fancy_recv_udp: fancy_recv_udp.o
	$(CC) -o fancy_recv_udp fancy_recv_udp.o

send_udp: send_udp.o
	$(CC) -o send_udp send_udp.o

clean:
	rm -f *.o recv_udp send_udp fancy_recv_udp
