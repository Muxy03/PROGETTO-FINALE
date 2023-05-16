#define _GNU_SOURCE  /* See feature_test_macros(7) */
#include <stdio.h>   // permette di usare scanf printf etc ...
#include <stdlib.h>  // conversioni stringa/numero exit() etc ...
#include <stdbool.h> // gestisce tipo bool (variabili booleane)
#include <assert.h>  // permette di usare la funzione assert
#include <string.h>  // confronto/copia/etc di stringhe
#include <errno.h>
#include <search.h>
#include <signal.h>
#include <unistd.h>   // per sleep
#include <sys/stat.h> /* For mode constants */
#include <fcntl.h>    /* For O_* constants */
#include <pthread.h>

#define Num_elem 1000000
#define PC_buffer_len 10

int lenhash = 0;
ENTRY *testa = NULL;
int *ht;

void termina(const char *messaggio)
{
    if (errno != 0)
        perror(messaggio);
    else
        fprintf(stderr, "%s\n", messaggio);
    exit(1);
}

ENTRY *crea_entry(char *s)
{
    ENTRY *e = malloc(sizeof(ENTRY));
    if (e == NULL)
        termina("errore malloc entry 1");
    e->key = strdup(s);
    e->data = (int *)malloc(sizeof(int));
    if (e->key == NULL || e->data == NULL)
        termina("errore malloc entry 2");
    coppia *c = (coppia *)e->data;
    c->valore = 1;
    c->next = NULL;
    return e;
}

void distruggi_entry(ENTRY *e)
{
    free(e->key);
    free(e->data);
    free(e);
}

typedef struct
{
    int valore;
    ENTRY *next;
} coppia;

void aggiungi(char *s)
{
    ENTRY *e = crea_entry(s);
    ENTRY *r = hsearch(*e, FIND);
    if (r == NULL)
    {
        r = hsearch(*e, ENTER);
        if (r == NULL)
        {
            termina("errore o tabella piena");
        }
        coppia *c = (coppia *)e->data;
        c->next = testa;
        testa = e;
        lenhash++;
    }
    else
    {
        assert(strcmp(e->key, r->key) == 0);
        coppia *c = (coppia *)r->data;
        c->valore += 1;
        distruggi_entry(e);
    }
}

int conta(char *s)
{
    ENTRY *e = crea_entry(s);
    ENTRY *r = hsearch(*e, FIND);
    if (r == NULL)
    {
        return 0;
    }
    else
    {
        assert(strcmp(e->key, r->key) == 0);
        int *d = (int *)r->data;
        int res = *d;
        distruggi_entry(e);
        return res;
    }
}

typedef struct
{
    char *nomePipe;
    char *buffer;
} argCapi;

void *gestioneScrittori(void *arg)
{
    // TODO: suddividere lavoro ai thread mediante buffer
    argCapi *a = (argCapi *)arg;
    int fd = open(a->nomePipe, O_RDONLY);
    if (fd == -1)
    {
        termina("errore apertura caposc\n");
    }
    ssize_t bytesRead;
    while (bytesRead = read(fd, a->buffer, PC_buffer_len) > 0)
    {
      /*
      Il thread "capo scrittore" legge il suo input da una FIFO (named pipe) caposc. 
      L'input che riceve sono sequenze di byte, ognuna preceduta dalla sua lunghezza. 
      Per ogni sequenza ricevuta il thread capo scrittore deve aggiungere in fondo un byte uguale a 0; 
      successivamente deve effettuare una tokenizzazione utilizzando strtok (o forse strtok_r?) utilizzando ".,:; \n\r\t" come stringa di delimitatori. 
      Una copia (ottenuta con strdup) di ogni token deve essere messo su un buffer produttori-consumatori per essere gestito dai thread scrittori (che svolgono il ruolo di consumatori)
      */   
    }
    close(fd);
    return NULL;
}

void *gestioneLettori(void *arg)
{
    // TODO: suddividere lavoro ai thread mediante buffer
    argCapi *a = (argCapi *)arg;
    int fd = open(a->nomePipe, O_RDONLY);
    if (fd == -1)
    {
        termina("errore apertura capolet\n");
    }
    ssize_t bytesRead;
    while (bytesRead = read(fd, a->buffer, PC_buffer_len) > 0)
    {
      
    }
    close(fd);
    return NULL;
}

int main(int argc, char *argv[])
{
    int w = atoi(argv[1]);
    int r = atoi(argv[2]);
    char bufferS[PC_buffer_len];
    char bufferL[PC_buffer_len];
    argCapi a[2]; // 0->CapoScrittori, 1->CapoLettori

    pthread_t scrittori[w];
    pthread_t lettori[w];
    pthread_t capoS; // capo Scrittore
    pthread_t capoL; // capo Lettore

    a[0].nomePipe = "caposc";
    a[0].buffer = &bufferS;
    a[1].nomePipe = "capolet";
    a[1].buffer = &bufferL;

    ht = hcreate(Num_elem);
    if (ht == 0)
    {
        termina("errore hcreate");
    }

    // TODO:thread gestore segnali

    pthread_create(&capoS, NULL, &gestioneScrittori, &a[0]);
    pthread_create(&capoL, NULL, &gestioneLettori, &a[1]);

    return 0;
}

/*
APPUNTI:

SERVER "A" -> SCRIVE NELLA FIFO "capolet"
SERVER "B" -> SCRIVE NELLA FIFO "caposc"

CAPO-SCRITTORE LEGGE "caposc"
CAPO-LETTORE LEGGE "capolet"

*/