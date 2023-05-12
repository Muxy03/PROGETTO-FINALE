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

int main(int argc, char *argv[])
{
    int scrittori = atoi(argv[1]);
    int lettori = atoi(argv[2]);
    int ht = hcreate(Num_elem);
    if (ht == 0)
    {
        termina("errore hcreate");
    }

    // TODO:thread gestore segnali

    // TODO: thread caposcrittore
    for (int i = 0; i < scrittori; i++)
    {
        // TODO: thread scrittori -> aggiungi
    }

    // TODO: thread capolettore
    for (int i = 0; i < lettori; i++)
    {
        // TODO: thread lettori -> conta
    }
    return 0;
}

/*
APPUNTI:

SERVER "A" -> SCRIVE NELLA FIFO "capolet"
SERVER "B" -> SCRIVE NELLA FIFO "caposc"

CAPO-SCRITTORE LEGGE "caposc"
CAPO-LETTORE LEGGE "capolet"

*/