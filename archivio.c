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

static int lenhash = 0;
static ENTRY *testa = NULL;

typedef struct
{
    int *nthread;
    char *fifo;
    char **buffer;
    int *index;
    pthread_mutex_t *mutex;
    pthread_cond_t *full;
    pthread_cond_t *empty;
    bool *continua;
    pthread_mutex_t *hashmutex;
    pthread_cond_t *hash;
} argCapi;

typedef struct
{
    int *index;
    char **buffer;
    pthread_mutex_t *mutex;
    pthread_cond_t *full;
    pthread_cond_t *empty;
    pthread_mutex_t *hashmutex;
    pthread_cond_t *hash;
} argT;

typedef struct
{
    int *lenhash;
    pthread_t *CS;
    pthread_t *CL;
    bool *continua;
    pthread_mutex_t *mutex;
    pthread_cond_t *hash;

} argSig;

typedef struct
{
    int valore; // numero di occorrenze della stringa
    ENTRY *next;
} coppia;

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
    {
        termina("errore malloc entry 1");
    }
    e->key = strdup(s); // salva copia di s
    e->data = (int *)malloc(sizeof(int));
    if (e->key == NULL || e->data == NULL)
    {
        termina("errore malloc entry 2");
    }
    coppia *c = (coppia *)e->data; // cast obbligatorio
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

void aggiungi(char *s)
{
    ENTRY *e = crea_entry(s);
    ENTRY *r = hsearch(*e, FIND);
    if (r == NULL)
    {
        r = hsearch(*e, ENTER);
        lenhash++;
        if (r == NULL)
        {
            termina("errore o tabella piena");
        }
        coppia *c = (coppia *)e->data;
        c->next = testa;
        testa = e;
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
        return *((int *)r->data);
    }
    distruggi_entry(e);
}

void *Scrittori(void *arg)
{
    printf("sono uno scrittore\n");
    argT *a = (argT *)arg;

    while (*(a->index) > 0)
    {
        pthread_mutex_lock(a->mutex);
        while (*(a->index) == 0)
        {
            pthread_cond_wait(a->full, a->mutex);
        }

        char *s = a->buffer[*(a->index)-- % PC_buffer_len];
        pthread_mutex_lock(a->hashmutex);
        aggiungi(s);
        pthread_cond_signal(a->hash);
        pthread_mutex_unlock(a->hashmutex);
        free(s);
        pthread_cond_signal(a->empty);
        pthread_mutex_unlock(a->mutex);
    }

    return NULL;
}

void *Lettori(void *arg)
{
    printf("sono un lettore\n");
    argT *a = (argT *)arg;
    FILE *f = fopen("lettori.log", "a");
    if (f == NULL)
    {
        termina("errore apertura lettore.log");
    }

    while (*(a->index) > 0)
    {
        pthread_mutex_lock(a->mutex);
        while (*(a->index) == 0)
        {
            pthread_cond_wait(a->full, a->mutex);
        }

        char *s = a->buffer[*(a->index)-- % PC_buffer_len];
        pthread_mutex_lock(a->hashmutex);
        int n = conta(s);
        pthread_cond_signal(a->hash);
        pthread_mutex_unlock(a->hashmutex);
        fprintf(f, "%s %d\n", s, n);
        free(s);
        pthread_cond_signal(a->empty);
        pthread_mutex_unlock(a->mutex);
    }
    return NULL;
}

void *capoScrittori(void *arg)
{
    printf("sono il capo scrittori\n");
    argCapi *a = (argCapi *)arg;
    int fd = open(a->fifo, O_RDONLY);
    if (fd == -1)
    {
        termina("errore apertura fifo");
    }
    int len, index = 0;
    char buffer[Max_sequence_length];
    char *saveptr;

    pthread_t t[*(a->nthread)];
    argT b[*(a->nthread)];
    printf("creo i scrittori\n");
    // for (int i = 0; i < *(a->nthread); i++)
    // {
    //     b[i].index = &index;
    //     b[i].buffer = a->buffer;
    //     b[i].mutex = a->mutex;
    //     b[i].full = a->full;
    //     b[i].empty = a->empty;
    //     b[i].hashmutex = a->hashmutex;
    //     b[i].hash = a->hash;
    //     pthread_create(&t[i], NULL, Scrittori, &b[i]);
    // }

    if (read(fd, &len, sizeof(int)) != sizeof(int))
    {
        termina("errore read");
    }

    if (read(fd, buffer, len) != len)
    {
        termina("errore read");
    }

    buffer[len] = '\0';

    char *token = strtok_r(buffer, ".,:; \n\r\t", &saveptr);

    while (token != NULL)
    {
        char *s = strdup(token);
        pthread_mutex_lock(a->mutex);
        while (*(a->index) - 1 == PC_buffer_len)
        {
            pthread_cond_wait(a->empty, a->mutex);
        }
        a->buffer[*(a->index)++ % PC_buffer_len] = s;

        pthread_cond_signal(a->full);
        pthread_mutex_unlock(a->mutex);

        pthread_mutex_lock(a->hashmutex);

        if (a->continua == false)
        {
            for (int i = 0; i < *(a->nthread); i++)
            {
                pthread_join(t[i], NULL);
            }
            pthread_cond_signal(a->hash);
            pthread_mutex_unlock(a->hashmutex);
            break;
        }

        token = strtok_r(NULL, ".,:; \n\r\t", &saveptr);
    }

    close(fd);
    return NULL;
}

void *capoLettori(void *arg)
{
    printf("sono il capo lettori\n");
    argCapi *a = (argCapi *)arg;
    int fd = open(a->fifo, O_RDONLY);
    if (fd == -1)
    {
        termina("errore apertura fifo");
    }
    int len, index = 0;
    char buffer[Max_sequence_length];
    char *saveptr;

    pthread_t t[*(a->nthread)];
    argT b[*(a->nthread)];

    printf("creo i lettori\n");
    for (int i = 0; i < *(a->nthread); i++)
    {
        b[i].index = &index;
        b[i].buffer = a->buffer;
        b[i].mutex = a->mutex;
        b[i].full = a->full;
        b[i].empty = a->empty;
        b[i].hashmutex = a->hashmutex;
        b[i].hash = a->hash;
        pthread_create(&t[i], NULL, Lettori, &b[i]);
    }

    if (read(fd, &len, sizeof(int)) != sizeof(int))
    {
        termina("errore read");
    }

    if (read(fd, buffer, len) != len)
    {
        termina("errore read");
    }

    buffer[len] = '\0';

    char *token = strtok_r(buffer, ".,:; \n\r\t", &saveptr);

    while (token != NULL)
    {
        char *s = strdup(token);
        pthread_mutex_lock(a->mutex);
        while (*(a->index) - 1 == PC_buffer_len)
        {
            pthread_cond_wait(a->empty, a->mutex);
        }
        a->buffer[*(a->index)++ % PC_buffer_len] = s;

        pthread_cond_signal(a->full);
        pthread_mutex_unlock(a->mutex);

        pthread_mutex_lock(a->hashmutex);

        if (a->continua == false)
        {
            for (int i = 0; i < *(a->nthread); i++)
            {
                pthread_join(t[i], NULL);
            }
            pthread_cond_signal(a->hash);
            pthread_mutex_unlock(a->hashmutex);
            break;
        }

        token = strtok_r(NULL, ".,:; \n\r\t", &saveptr);
    }

    close(fd);
    return NULL;
}

void *sigHandler(void *arg)
{
    printf("sono il sigHandler\n");
    argSig *a = (argSig *)arg;
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGINT);
    sigaddset(&set, SIGTERM);
    sigaddset(&set, SIGUSR1);
    pthread_sigmask(SIG_BLOCK, &set, NULL);

    int s;

    while (*(a->continua))
    {
        int e = sigwait(&set, &s);
        if (e != 0)
        {
            termina("errore sigwait");
        }
        if (s == SIGINT)
        {
            fprintf(stderr, "# stringhe distinte nella tabella hash :%d\n", *(a->lenhash));
        }
        if (s == SIGTERM)
        {
            printf("sono in sigterm\n");
            *(a->continua) = false;
            pthread_join(*(a->CS), NULL);
            pthread_join(*(a->CL), NULL);
            fprintf(stdout, "# stringhe distinte nella tabella hash :%d\n", *(a->lenhash));

            pthread_mutex_lock(a->mutex);
            ENTRY *current = testa;
            ENTRY *next;

            while (current != NULL)
            {
                next = ((coppia*)current->data)->next;
                distruggi_entry(current);
                current = next;
            }
            hdestroy();
            pthread_cond_signal(a->hash);
            pthread_mutex_unlock(a->mutex);
        }
        else if (s == SIGUSR1)
        {
            pthread_mutex_lock(a->mutex);
            ENTRY *current = testa;
            ENTRY *next;

            while (current != NULL)
            {
                next = ((coppia*)current->data)->next;
                distruggi_entry(current);
                current = next;
            }
            hdestroy();
            hcreate(Num_elem);
            pthread_cond_signal(a->hash);
            pthread_mutex_unlock(a->mutex);
        }
    }

    return NULL;
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        termina("Uso: ./archivio <w> <r>\n");
    }
    int hashtable = hcreate(Num_elem);
    if (hashtable == 0)
    {
        termina("Errore hcreate\n");
    }
    int w = atoi(argv[1]);
    int r = atoi(argv[2]);
    int indexS = 0;
    int indexL = 0;

    pthread_mutex_t mutexS = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_t mutexL = PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_t fullS = PTHREAD_COND_INITIALIZER;
    pthread_cond_t emptyS = PTHREAD_COND_INITIALIZER;
    pthread_cond_t fullL = PTHREAD_COND_INITIALIZER;
    pthread_cond_t emptyL = PTHREAD_COND_INITIALIZER;
    char *bufferS[PC_buffer_len];
    char *bufferL[PC_buffer_len];

    pthread_mutex_t mutexH = PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_t hash = PTHREAD_COND_INITIALIZER;
    bool continua = false;

    argSig arg;
    argCapi args[2]; // 0 -> scrittori, 1 -> lettori
    pthread_t capi[2];
    pthread_t sigH;

    args[0].nthread = &w;
    args[0].fifo = "caposc";
    args[0].buffer = bufferS;
    args[0].index = &indexS;
    args[0].mutex = &mutexS;
    args[0].full = &fullS;
    args[0].empty = &emptyS;
    args[0].hash = &hash;
    args[0].hashmutex = &mutexH;

    args[1].nthread = &r;
    args[1].fifo = "capolet";
    args[1].buffer = bufferL;
    args[1].index = &indexL;
    args[1].mutex = &mutexL;
    args[1].full = &fullL;
    args[1].empty = &emptyL;
    args[1].hash = &hash;
    args[1].hashmutex = &mutexH;

    arg.lenhash = &lenhash;
    arg.mutex = &mutexH;
    arg.hash = &hash;
    arg.continua = &continua;
    arg.CS = &capi[0];
    arg.CL = &capi[1];

    pthread_create(&capi[0], NULL, &capoScrittori, &args[0]);
    pthread_create(&capi[1], NULL, &capoLettori, &args[1]);
    pthread_create(&sigH, NULL, &sigHandler, &arg);

    pthread_join(sigH, NULL);

    printf("termino\n");

    return 0;
}
