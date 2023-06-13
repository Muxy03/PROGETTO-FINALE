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

int fine = 0;
int H_used = 0;

typedef struct
{
    char **buffer;
    int *counter;
    pthread_mutex_t *hmutex;  // hash table
    pthread_mutex_t *bfmutex; // buffer PC
    pthread_cond_t *hcond;    // hash table
    pthread_cond_t *empty;    // buffer PC
    pthread_cond_t *full;     // buffer PC
    pthread_mutex_t *fmutex;  // lettori.log mutex
} SLarg;

typedef struct
{
    char *fifo;
    char **buffer;
    int *counter;
    pthread_mutex_t *bfmutex; // buffer PC
    pthread_cond_t *empty;    // buffer PC
    pthread_cond_t *full;     // buffer PC
} CSLarg;

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
    SLarg *a = (SLarg *)args;

    while (1)
    {
        while (*(a->counter) == 0)
        {
            if (fine)
            {
                break;
            }
            pthread_cond_wait(a->full, a->bfmutex);
        }

        if (*(a->counter) == 0 && fine)
        {
            break;
        }

        char *token = a->buffer[*(a->counter)-- % PC_buffer_len];
        pthread_mutex_unlock(a->bfmutex);
        pthread_cond_signal(a->empty);
        pthread_mutex_lock(a->hmutex);

        while (H_used == 1)
        {
            pthread_cond_wait(a->hcond, a->hmutex);
        }
        aggiungi(token);
        pthread_mutex_unlock(a->hmutex);
        H_used = 0;
        pthread_cond_broadcast(a->hcond);
    }
    return NULL;
}

void *Lettori(void *args)
{
    SLarg *a = (SLarg *)args;

    while (1)
    {
        while (*(a->counter) == 0)
        {
            if (fine)
            {
                break;
            }
            pthread_cond_wait(a->full, a->bfmutex);
        }

        if (*(a->counter) == 0 && fine)
        {
            break;
        }

        char *token = a->buffer[*(a->counter)-- % PC_buffer_len];
        pthread_mutex_unlock(a->bfmutex);
        pthread_cond_signal(a->empty);
        pthread_mutex_lock(a->hmutex);

        while (H_used == 1)
        {
            pthread_cond_wait(a->hcond, a->hmutex);
        }

        int tmp = conta(token);
        pthread_mutex_unlock(a->hmutex);
        H_used = 0;
        pthread_cond_broadcast(a->hcond);
        pthread_mutex_lock(a->fmutex);
        FILE *f = fopen("lettori.log", "a");
        fprintf(f, "%s %d\n", token, tmp);
        fflush(f);
    }
    return NULL;
}

void *Capi(void *args)
{
    CSLarg *a = (CSLarg *)args;
    int fd = open(a->fifo, O_RDWR);

    char length[4];
    char *data;
    char *token;
    char *saveptr;
    while (read(fd, length, sizeof(length)) > 0)
    {
        int len = atoi(length);
        data = malloc((len + 1) * sizeof(char));
        read(fd, data, len);
        data[len] = '\0';

        token = strtok_r(data, ".,:; \n\r\t", &saveptr);
        while (token != NULL)
        {
            pthread_mutex_lock(a->bfmutex);
            while (*(a->counter) == PC_buffer_len)
            {
                pthread_cond_wait(a->empty, a->bfmutex);
            }
            a->buffer[*(a->counter)++ % PC_buffer_len] = strdup(token);
            pthread_mutex_unlock(a->bfmutex);
            pthread_cond_signal(a->full);
            token = strtok_r(NULL, ".,:; \n\r\t", &saveptr);
        }
    }
    free(length);
    free(data);
    free(token);
    free(saveptr);
    close(fd);
    return NULL;
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

    char *fifoL = "capolet";
    char *fifoS = "caposc";
    pthread_mutex_t hmutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_t hcond = PTHREAD_COND_INITIALIZER;

    char *PCbufferS[Max_sequence_length];
    int PCcounterS = 0;
    pthread_mutex_t bfmutexS = PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_t emptyS = PTHREAD_COND_INITIALIZER;
    pthread_cond_t fullS = PTHREAD_COND_INITIALIZER;

    char *PCbufferL[Max_sequence_length];
    int PCcounterL = 0;
    pthread_mutex_t bfmutexL = PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_t emptyL = PTHREAD_COND_INITIALIZER;
    pthread_cond_t fullL = PTHREAD_COND_INITIALIZER;

    SLarg argS[scrittori];
    SLarg argL[lettori];

    CSLarg argC[2];
    argC[0].fifo = fifoL;
    argC[0].bfmutex = &bfmutexL;
    argC[0].buffer = PCbufferL;
    argC[0].counter = &PCcounterL;
    argC[0].empty = &emptyL;
    argC[0].full = &fullL;

    argC[1].fifo = fifoS;
    argC[1].bfmutex = &bfmutexS;
    argC[1].buffer = PCbufferS;
    argC[1].counter = &PCcounterS;
    argC[1].empty = &emptyS;
    argC[1].full = &fullS;

    pthread_create(&capi[0], NULL, Capi, &argC[0]);
    pthread_create(&capi[1], NULL, Capi, &argC[1]);

    for (int i = 0; i < scrittori; i++)
    {
        argS[i].bfmutex = &bfmutexS;
        argS[i].counter = &PCcounterS;
        argS[i].hmutex = &hmutex;
        argS[i].hcond = &hcond;
        argS[i].empty = &emptyS;
        argS[i].full = &fullS;
        argS[i].buffer = PCbufferS;
        pthread_create(&writers[i], NULL, Scrittori, &argS[i]);
    }

    for (int i = 0; i < lettori; i++)
    {
        argL[i].bfmutex = &bfmutexL;
        argL[i].counter = &PCcounterL;
        argL[i].hmutex = &hmutex;
        argL[i].hcond = &hcond;
        argL[i].empty = &emptyL;
        argL[i].full = &fullL;
        argL[i].buffer = PCbufferL;
        pthread_create(&readers[i], NULL, Lettori, &argL[i]);
    }

    return 0;
}