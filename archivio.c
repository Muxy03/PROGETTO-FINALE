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
volatile bool continua = true;

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
    int *counter;
    int *index;
    int *nthread;
    pthread_mutex_t *mutex;
    pthread_cond_t *full;
    pthread_cond_t *empty;
    bool *closed;
} argCapi;

typedef struct
{
    char *nomePipe;
    char *buffer;
    int *counter;
    int *in;
    int *out;
    pthread_mutex_t *mutex;
    pthread_cond_t *full;
    pthread_cond_t *empty;
    bool *closed;
} argT;

typedef struct
{
    int lehash;
    bool continua;
    pthread_t *capoS;
    pthread_t *capoL;
    pthread_mutex_t *mutex;
} argSig;

void *gestioneScrittori(void *arg)
{
    // TODO: suddividere lavoro ai thread mediante buffer
    argCapi *a = (argCapi *)arg;
    pthread_t t[*(a->nthread)];
    argT b[*(a->nthread)];

    for (int i = 0; i < *(a->nthread); i++)
    {
        b[i].nomePipe = a->nomePipe;
        b[i].buffer = a->buffer;
        b[i].counter = a->counter;
        b[i].in = 0;
        b[i].empty = a->empty;
        b[i].full = a->full;
        b[i].mutex = a->mutex;
        b[i].out = 0;
        b[i].closed = a->closed;

        pthread_create(&t[i], NULL, &Scrittori, &b[i]);
    }
    int fd = open(a->nomePipe, O_RDONLY);
    if (fd == -1)
    {
        termina("errore apertura caposc\n");
    }
    char lenBuffer[sizeof(int)];
    // Lettura delle sequenze di byte dalla FIFO
    while (read(fd, lenBuffer, sizeof(int)) > 0)
    {
        // Conversione della lunghezza in intero
        int sequenceLen = *(int *)lenBuffer;

        // Buffer per leggere la sequenza di byte
        char sequenceBuffer[sequenceLen];

        // Lettura della sequenza di byte dalla FIFO
        if (read(fd, sequenceBuffer, sequenceLen) > 0)
        {
            // Aggiunta del byte 0 alla fine della sequenza
            sequenceBuffer[sequenceLen] = '\0';

            // Tokenizzazione della sequenza
            char *token;
            char *saveptr;
            token = strtok_r(sequenceBuffer, ".,:; \n\r\t", &saveptr);

            // Inserimento dei token nel buffer produttore-consumatore
            while (token != NULL)
            {
                pthread_mutex_lock(&a->mutex);

                // Attendere se il buffer è pieno
                while (a->counter == PC_buffer_len && !(*(a->closed)))
                    pthread_cond_wait(&a->empty, &a->mutex);

                if (a->closed == true)
                {
                    pthread_mutex_unlock(&a->mutex);
                    *(a->closed) = false;
                    break;
                }
                else
                {
                    *(a->closed) = true;
                }

                // Inserire il token nel buffer
                a->buffer[*(a->index)++ % PC_buffer_len] = strdup(token);
                *(a->counter)++;

                // Svegliare un thread scrittore in attesa
                pthread_cond_signal(&a->full);

                pthread_mutex_unlock(&a->mutex);

                // Ottenere il prossimo token
                token = strtok_r(NULL, ".,:; \n\r\t", &saveptr);
            }
        }
        free(lenBuffer);
        free(sequenceBuffer);
    }
    close(fd);
    return NULL;
}

void *Scrittori(void *arg)
{
    argT *a = (argT *)arg;

    while (*(a->counter) < PC_buffer_len)
    {
        pthread_mutex_lock(&a->mutex);

        while (*(a->counter) == PC_buffer_len && !(*(a->closed)))
            pthread_cond_wait(&a->empty, &a->mutex);

        if (a->closed == true)
        {
            pthread_mutex_unlock(&a->mutex);
            *(a->closed) = false;
            break;
        }
        else
        {
            *(a->closed) = true;
        }

        char *token = a->buffer[*(a->in)++ % PC_buffer_len];
        a->counter++;

        pthread_cond_signal(&a->full);
        pthread_mutex_unlock(&a->mutex);
        aggiungi(token);
        free(token);
    }

    return NULL;
}

void *Lettori(void *arg)
{
    argT *a = (argT *)arg;
    FILE *lettori = fopen("lettori.log", "a");

    while (*(a->counter) > 0)
    {
        pthread_mutex_lock(&a->mutex);

        while (*(a->counter) == 0 && !(*(a->closed)))
            pthread_cond_wait(&a->full, &a->mutex);

        if (a->closed == true)
        {
            pthread_mutex_unlock(&a->mutex);
            break;
        }

        char *token = a->buffer[*(a->out)++ % PC_buffer_len];
        a->counter--;

        pthread_cond_signal(&a->empty);
        pthread_mutex_unlock(&a->mutex);
        int value = conta(token);
        fprintf(lettori, "%s %d\n", token, value);
        fflush(lettori);
        free(token);
    }

    return NULL;
}

void *gestioneLettori(void *arg)
{
    argCapi *a = (argCapi *)arg;
    pthread_t t[*(a->nthread)];
    argT b[*(a->nthread)];

    for (int i = 0; i < *(a->nthread); i++)
    {
        b[i].nomePipe = a->nomePipe;
        b[i].buffer = a->buffer;
        b[i].counter = a->counter;
        b[i].in = 0;
        b[i].empty = a->empty;
        b[i].full = a->full;
        b[i].mutex = a->mutex;
        b[i].out = 0;
        b[i].closed = a->closed;

        pthread_create(&t[i], NULL, &Scrittori, &b[i]);
    }

    int fd = open(a->nomePipe, O_RDONLY);
    if (fd == -1)
    {
        termina("errore apertura capolet\n");
    }
    char lenBuffer[sizeof(int)];
    // Lettura delle sequenze di byte dalla FIFO
    while (read(fd, lenBuffer, sizeof(int)) > 0)
    {
        // Conversione della lunghezza in intero
        int sequenceLen = *(int *)lenBuffer;

        // Buffer per leggere la sequenza di byte
        char sequenceBuffer[sequenceLen];

        // Lettura della sequenza di byte dalla FIFO
        if (read(fd, sequenceBuffer, sequenceLen) > 0)
        {
            // Aggiunta del byte 0 alla fine della sequenza
            sequenceBuffer[sequenceLen] = '\0';

            // Tokenizzazione della sequenza
            char *token;
            char *saveptr;
            token = strtok_r(sequenceBuffer, ".,:; \n\r\t", &saveptr);

            // Inserimento dei token nel buffer produttore-consumatore
            while (token != NULL)
            {
                pthread_mutex_lock(&a->mutex);

                // Attendere se il buffer è pieno
                while (a->counter == PC_buffer_len && !(*(a->closed)))
                    pthread_cond_wait(&a->empty, &a->mutex);

                if (a->closed == true)
                {
                    pthread_mutex_unlock(&a->mutex);
                    break;
                }

                // Inserire il token nel buffer
                a->buffer[*(a->index)++ % PC_buffer_len] = strdup(token);
                *(a->counter)++;

                // Svegliare un thread scrittore in attesa
                pthread_cond_signal(&a->full);

                pthread_mutex_unlock(&a->mutex);

                // Ottenere il prossimo token
                token = strtok_r(NULL, ".,:; \n\r\t", &saveptr);
            }
        }
        free(lenBuffer);
        free(sequenceBuffer);
    }
    close(fd);
    return NULL;
}

void *sigHandler(void *arg)
{
    argSig *a = (argSig *)arg;
    sigset_t mask;
    sigfillset(&mask);
    sigdelset(&mask, SIGINT);
    int s;
    while (true)
    {
        int e = sigwait(&mask, &s);
        if (e != 0)
            perror("Errore sigwait");
        printf("Thread gestore svegliato dal segnale %d\n", s);
        if (s == SIGINT)
        {
            fprintf(stderr, "Nella tabella hash sono presenti :%d stringhe distinte\n", a->lehash);
        }
        if (s == SIGTERM)
        {
            pthread_join(&a->capoS, NULL);
            pthread_join(&a->capoL, NULL);

            fprintf(stdout, "Nella tabella hash sono presenti :%d stringhe distinte\n", a->lehash);

            ENTRY *curr = testa;

            while (curr != NULL)
            {
                ENTRY *temp = curr;
                coppia *c = (coppia *)temp->data;
                curr = c->next;
                distruggi_entry(temp);
            }

            hdestroy();

            exit(0);
        }

        if (s == SIGUSR1)
        {
            pthread_mutex_lock(&a->mutex);
            
            ENTRY *curr = testa;
            while (curr != NULL)
            {
                ENTRY *temp = curr;
                coppia *c = (coppia *)temp->data;
                curr = c->next;
                distruggi_entry(temp);
            }

            hdestroy();
            hcreate(Num_elem);
            a->lehash = 0;
            pthread_mutex_unlock(&a->mutex);
        }
    }
    return NULL;
}

int main(int argc, char *argv[])
{
    int w = atoi(argv[1]);
    int r = atoi(argv[2]);
    char bufferS[PC_buffer_len];
    char bufferL[PC_buffer_len];
    argCapi a[2]; // 0->CapoScrittori, 1->CapoLettori
    argSig b;

    pthread_t scrittori[w];
    pthread_t lettori[w];
    pthread_t capoS; // capo Scrittore
    pthread_t capoL; // capo Lettore
    pthread_t gestore;
    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_t full = PTHREAD_COND_INITIALIZER;
    pthread_cond_t empty = PTHREAD_COND_INITIALIZER;
    int counterS = 0;
    int counterL = 0;
    int indexS = 0;
    int indexL = 0;
    bool closed = false;

    a[0].nomePipe = "caposc";
    a[0].buffer = &bufferS;
    a[0].mutex = &mutex;
    a[0].full = &full;
    a[0].empty = &empty;
    a[0].counter = &counterS;
    a[0].index = &indexS;
    a[0].closed = &closed;
    a[1].nomePipe = "capolet";
    a[1].buffer = &bufferL;
    a[1].mutex = &mutex;
    a[1].full = &full;
    a[1].empty = &empty;
    a[1].counter = &counterL;
    a[1].index = &indexL;
    a[1].closed = &closed;

    b.capoS = &capoS;
    b.capoL = &capoL;
    b.continua = &continua;
    b.lehash = &lenhash;
    b.mutex = &mutex;
    ht = hcreate(Num_elem);
    if (ht == 0)
    {
        termina("errore hcreate");
    }

    // TODO:thread gestore segnali

    pthread_create(&capoS, NULL, &gestioneScrittori, &a[0]);
    pthread_create(&capoL, NULL, &gestioneLettori, &a[1]);
    pthread_create(&gestore, NULL, &sigHandler, &b);

    return 0;
}

/*
APPUNTI:

SERVER "A" -> SCRIVE NELLA FIFO "capolet"
SERVER "B" -> SCRIVE NELLA FIFO "caposc"

CAPO-SCRITTORE LEGGE "caposc"
CAPO-LETTORE LEGGE "capolet"

*/