#Progetto Finale Laboratorio II

Scelte implementative:
1. connessione di tipo A  -> il client manda la stringa "1"
2. connessione di tipo B -> il client manda la stringa "0"
3. terminazione thread in archivio -> scrittura nelle fifo di "0000" e aggiornamento variabile fine (da 0 a 1)
4. per archivio ho creato 5 strutture dati:
	1. Buffer -> struttura per la gestione dei buffer
	2. Gargs -> variabili per il funzionamento di Gestore
	3. SLargs -> analogo di Gargs ma per Scrittore e Lettore
	4. Cargs -> analogo di Gargs ma per Capo-Scrittore e Capo-Lettore
	5. Node -> struttura per la creazione di Nodi della Linked List
5. Linked List:
	1. funzione Enqueue -> funzione che aggiorna la lista spostando il puntatore della testa sul nuovo nodo
	2. funzione Distruggi_lista -> funzione che visita ogni nodo della lista e ad ogni iterazione libera la memoria del nodo visitato
