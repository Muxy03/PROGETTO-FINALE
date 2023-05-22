CC=gcc
CFLAGS=-std=c11 -Wall -g -O -pthread
LDLIBS=-lm -lrt -pthread

EXECS = archivio client1 client2

all: $(EXECS)

%: %.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

%.o: %.c 
	$(CC) $(CFLAGS) -c $<

clear:
	rm -f *.o $(EXECS)