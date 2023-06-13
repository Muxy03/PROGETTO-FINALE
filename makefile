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
EXECS= client1 client2 archivio

# primo target: gli eseguibili sono precondizioni
# quindi verranno tutti creati
all: $(EXECS) 


# regola per la creazioni degli eseguibili utilizzando xerrori.o
%: %.o 
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)


# regola per la creazione di file oggetto che dipendono da xerrori.h
%.o: %.c 
	$(CC) $(CFLAGS) -c $<

 
# esempio di target che non corrisponde a una compilazione
# ma esegue la cancellazione dei file oggetto e degli eseguibili
clean: 
	rm -f *.o $(EXECS)
	
# crea file zip della lezione	
zip:
	zip threads.zip *.c *.h *.py makefile


clear:
	rm -f *.o $(EXECS)