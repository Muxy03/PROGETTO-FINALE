# definizione del compilatore e dei flag di compilazione
# che vengono usate dalle regole implicite
CC=gcc
CFLAGS=-std=c11 -Wall -g -O -pthread
LDLIBS=-lm -lrt -pthread


# su https://www.gnu.org/software/make/manual/make.html#Implicit-Rules
# sono elencate le regole implicite e le variabili 
# usate dalle regole implicite 

# Variabili automatiche: https://www.gnu.org/software/make/manual/make.html#Automatic-Variables
# nei comandi associati ad ogni regola:
#  $@ viene sostituito con il nome del target
#  $< viene sostituito con il primo prerequisito
#  $^ viene sostituito con tutti i prerequisiti

# elenco degli eseguibili da creare
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
	rm -f *.o
 
# esempio di target che non corrisponde a una compilazione
# ma esegue la cancellazione dei file oggetto e degli eseguibili
clean: 
	rm -f *.o $(EXECS) capolet caposc valgrind* server.log lettori.log
	
# crea file zip della lezione	
zip:
	zip threads.zip *.c *.h *.py makefile

