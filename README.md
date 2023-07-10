# Progetto Finale Laboratorio II

Scelte implementative:

1. All'interno di progetto.h vi sono tutti gli include, le costanti (HOST,PORT), le strutture dati più importanti e le funzioni più importanti (ovviamente le definizioni dei prototipi delle funzioni sono in progetto.c)

2. Nel server scrivo la lunghezza della riga ricevuta tramite connessione socket nella fifo d'interesse, prima di scrivere nella medesima fifo la riga stessa, mediante: "0"*(4-len(str(lunghezza))) + str(lunghezza) -> questa formula mi permette di scrivere la lunghezza con 4 cifre sempre (ex: riga = ciao => lunghezza = 4 => "0004" = lunghezza scritta nella fifo). Ovviamente i vari capi convertiranno questa lunghezza espressa come stringa in un intero per le operazioni successive (mediante funzione atoi);

3. Connessione di tipo A  -> il client manda la stringa "0"

4. Connessione di tipo B -> il client manda la stringa "1"

5. Terminazione thread (Scrittori, Lettori e rispettivi Capi) in archivio -> scrittura nelle fifo di "0000" e aggiornamento variabile fine (da 0 a 1)

6. Per archivio ho creato 5 strutture dati:
	1. Buffer -> struttura per la gestione dei buffer
	2. GArg -> variabili per il funzionamento di Gestore
	3. SLArg -> analogo di Gargs ma per Scrittore e Lettore
	4. CArg -> analogo di Gargs ma per Capo-Scrittore e Capo-Lettore
	5. Node -> struttura per la creazione di Nodi della Linked List

7. Linked List:
	1. funzione Enqueue -> funzione che aggiorna la lista spostando il puntatore della testa sul nuovo nodo
	2. funzione Distruggi_lista -> funzione che visita ogni nodo della lista e ad ogni iterazione libera la memoria del nodo visitato

8. All'interno del Gestore dei segnali utilizzo le funzioni printf e fprintf (funzioni non async-signal-safe) perché tutte le operazioni che modificano la variabile globale tot, a cui esse accedono solo in lettura, sono sincronizzate mediante il mutex ht_mutex quindi non dovrebberò generare comportamenti anomali. -> il mutex ht_mutex è inoltre usato per sincronizzare le operazioni che modificano la Linked List, sia dentro che fuori il Gestore dei segnali.
