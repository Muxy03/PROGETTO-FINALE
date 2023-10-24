CC=gcc
CFLAGS=-std=c11 -Wall -g -O -pthread
LDLIBS=-lm -lrt -pthread

EXECS= archivio client1 client2 pulizia
all: $(EXECS) 

archivio: archivio.o progetto.o
archivio.o: archivio.c progetto.h

client2: client2.o progetto.o
client2.o: client2.c progetto.h

client1: client1.o progetto.o
client1.o: client1.c progetto.h

progetto.o: progetto.c progetto.h

pulizia:
	rm -f *.o && chmod +x server.py

server:
	./server.py 5 -r 2 -w 4 -v

clean: 
	rm -f *.o $(EXECS) capolet caposc valgrind* server.log lettori.log

zip:
	zip threads.zip *.c *.h *.py makefile

