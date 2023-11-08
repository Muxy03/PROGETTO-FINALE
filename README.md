# Progetto Finale Laboratorio II

Scelte implementative:

1. All'interno di progetto.h vi sono tutti gli include, le costanti (ex: HOST,PORT), le strutture dati più importanti e le funzioni più importanti (ovviamente le definizioni dei prototipi delle funzioni sono in progetto.c)

2. Nel server scrivo la lunghezza della riga ricevuta tramite connessione socket nella fifo d'interesse, prima di scrivere nella medesima fifo la riga stessa. Ovviamente i vari capi convertiranno questa lunghezza espressa come stringa in un intero per le operazioni successive (mediante funzione atoi);

3. Connessioni dei client -> per evitare di mandare righe di lunghezza maggiore di 0 ma non composte da caratteri effettuo il controllo isspace(line\[0\]), assumendo che le linee valide inizino con un carattere ASCII:
	1. Connessione di tipo A  -> il client manda la stringa "0"
	2. Connessione di tipo B -> il client manda la stringa "1"

5. Terminazione thread (Scrittori, Lettori e rispettivi Capi) in archivio -> scrittura nelle fifo di "2049" e aggiornamento variabile fine (da 0 a 1)

6. Per archivio ho creato 5 strutture dati:
	1. Buffer -> struttura per la gestione dei buffer
	2. GArg -> variabili per il funzionamento di Gestore
	3. SLArg -> analogo di Gargs ma per Scrittore e Lettore
	4. CArg -> analogo di Gargs ma per Capo-Scrittore e Capo-Lettore
	5. Node -> struttura per la creazione di Nodi della Linked List

7. Linked List:
	1. SCHEMA AGGIORNAMENTO LINKED LIST:
		- Nodo1->NULL  -->  Nodo2->Nodo1->NULL
	2. funzione Enqueue -> funzione che aggiorna la lista (esempio del funzionamento nel punto 1)
	3. funzione Distruggi_lista -> funzione che visita ogni nodo della lista e ad ogni iterazione libera la memoria del nodo visitato

8. All'interno del Gestore dei segnali utilizzo le funzioni printf e fprintf (funzioni non async-signal-safe) perché tutte le operazioni, sia di lettura che di scrittura, della variabile globale tot sono sincronizzate mediante il mutex ht_mutex quindi non dovrebbero generare comportamenti anomali. Il mutex ht_mutex è inoltre usato per sincronizzare le operazioni che modificano la Linked List, sia dentro che fuori il Gestore dei segnali.
