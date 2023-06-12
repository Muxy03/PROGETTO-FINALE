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
#define Max_sequence_length 2048

typedef struct
{
    char **buffer;
    int counter;
    pthread_mutex_t *hmutex;  // hash table
    pthread_mutex_t *bfmutex; // buffer PC
    pthread_cond_t hcond;     // hash table
    pthread_cond_t empty;     // buffer PC
    pthread_cond_t full;      // buffer PC
} SLarg;

void termina(const char *messaggio)
{
    if (errno == 0)
        fprintf(stderr, "== %d == %s\n", getpid(), messaggio);
    else
        fprintf(stderr, "== %d == %s: %s\n", getpid(), messaggio,
                strerror(errno));
    exit(1);
}

ENTRY *crea_entry(char *s)
{
    ENTRY *e = malloc(sizeof(ENTRY));
    if (e == NULL)
    {
        termina("errore malloc entry 1");
    }
    e->key = strdup(s); // salva copia di s
    e->data = (int *)malloc(sizeof(int));
    if (e->key == NULL || e->data == NULL)
    {
        termina("errore malloc entry 2");
        *((int *)e->data) = 1;
        return e;
    }
}

void distruggi_entry(ENTRY *e)
{
    free(e->key);
    free(e->data);
    free(e);
}

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
    }
    else
    {
        assert(strcmp(e->key, r->key) == 0);
        int *TMP = (int *)r->data;
        *TMP += 1;
        distruggi_entry(e);
    }
}

int conta(char *s)
{
    ENTRY *e = crea_entry(s);
    ENTRY *r = hsearch(*e, FIND);
    int n = 0;
    if (r != NULL)
    {
        assert(strcmp(e->key, r->key) == 0);
        n = *((int *)r->data);
    }
    distruggi_entry(e);
    return n;
}

void *Scrittori(void *args)
{
}

void *Lettori(void *args)
{
}

void *Capi(void *args)
{
}

void *Signal_handler(void *args)
{
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        termina("Uso: ./archivio <w> <r>\n");
    }

    int scrittori = atoi(argv[1]);
    int lettori = atoi(argv[2]);

    pthread_t writers[scrittori];
    pthread_t readers[lettori];
    pthread_t capi[2]; // 0: lettori, 1: scrittori

    return 0;
}