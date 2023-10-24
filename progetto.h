#define _GNU_SOURCE  /* See feature_test_macros(7) */
#include <stdio.h>   // permette di usare scanf printf etc ...
#include <stdlib.h>  // conversioni stringa/numero exit() etc ...
#include <stdbool.h> // gestisce tipo bool (variabili booleane)
#include <assert.h>  // permette di usare la funzione assert
#include <string.h>  // confronto/copia/etc di stringhe
#include <errno.h>
#include <search.h>
#include <signal.h>
#include <unistd.h> // per sleep
#include <fcntl.h>  /* For O* constants */
#include <pthread.h>
#include <ctype.h>
#include <arpa/inet.h>
#include <sys/stat.h> /* For mode constants */
#include <sys/socket.h>

// CONSTANTI COMUNI
#define Num_elem 1000000
#define PC_buffer_len 10
#define Max_sequence_length 2048
#define HOST "127.0.0.1"
#define PORT 57943 // 637943

// STRUTTURE DATI
typedef struct
{
    char (*buffer)[Max_sequence_length];
    int *in;
    int *out;
    int *counter;
    pthread_mutex_t *mutex;
    pthread_cond_t *full;
    pthread_cond_t *empty;
} Buffer;

typedef struct
{
    pthread_t *Scrittori;
    pthread_t *Lettori;
    pthread_t *capoS;
    pthread_t *capoL;
    int w;
    int r;
    int *fine;
    sigset_t *set;
    pthread_cond_t *S_full;
    pthread_cond_t *L_full;
    pthread_mutex_t *ht_mutex;
} GArg;

typedef struct
{
    Buffer *bf;
} CArg;

typedef struct
{
    Buffer *bf;
    int *fine;
    pthread_mutex_t *hmutex;
    pthread_mutex_t *fmutex; // mutex necessario solo ai Lettori
} SLArg;

typedef struct
{
    ENTRY *e;
    struct Node *next;
} Node;

// FUNZIONI
void termina(const char *messaggio);

void Enqueue(ENTRY *e);

ENTRY *crea_entry(char *s, int n);

void distruggi_entry(ENTRY *e);

void aggiungi(char *s);

int conta(char *s);

void Distruggi_lista(Node *head);

void *CapoScrittore(void *arg);

void *CapoLettore(void *arg);

void *Gestore(void *arg);

void *Scrittore(void *arg);

void *Lettore(void *arg);
