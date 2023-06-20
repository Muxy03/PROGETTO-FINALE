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
#include <sys/stat.h> /* For mode constants */

// CONSTANTI
#define Num_elem 1000000
#define PC_buffer_len 10
#define Max_sequence_length 2048

// STRUTTURE DATI
typedef struct
{
    char *buffer[PC_buffer_len];
    int in;
    int out;
    int counter;
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
} Gargs;

typedef struct
{
    Buffer *bf;
} Cargs;

typedef struct
{
    Buffer *bf;
    int *fine;
    pthread_mutex_t *hmutex;
    pthread_mutex_t *fmutex;
} SLargs;

typedef struct
{
    ENTRY *e;
    struct Node *next;
} Node;

// VARIABILI GLOBALI (minimo indispensabile)
static volatile int tot = 0;
static Node *head = NULL;

// FUNZIONI
void termina(const char *messaggio)
{
    if (errno != 0)
    {
        perror(messaggio);
    }
    else
    {
        fprintf(stderr, "%s\n", messaggio);
    }
    exit(1);
}

void Enqueue(ENTRY *e)
{
    // FUNZIONE CHE AGGIUNGE UN ELEMENTO ALLA LISTA AGGIORNANDO IL PUNTATORE ALLA TESTA

    if (head == NULL)
    {
        head = malloc(sizeof(Node));
        if (head == NULL)
            termina("errore malloc head");
        head->e = e;
        head->next = NULL;
    }
    else
    {
        Node *tmp = malloc(sizeof(Node));
        if (tmp == NULL)
            termina("errore malloc tmp");
        tmp->e = e;
        tmp->next = (struct Node *)head;
        head = tmp;
    }
}

ENTRY *crea_entry(char *s, int n)
{
    ENTRY *e = malloc(sizeof(ENTRY));
    if (e == NULL)
    {
        termina("errore malloc entry 1");
    }
    e->key = strdup(s);
    e->data = (int *)malloc(sizeof(int));
    if (e->key == NULL || e->data == NULL)
    {
        termina("errore malloc entry 2");
    }
    *((int *)e->data) = n;
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
    ENTRY *e = crea_entry(s, 1);
    ENTRY *r = hsearch(*e, FIND);
    if (r == NULL)
    {
        r = hsearch(*e, ENTER);
        tot++;
        if (r == NULL)
        {
            termina("errore o tabella piena");
        }
        Enqueue(e);
    }
    else
    {
        assert(strcmp(e->key, r->key) == 0);
        int *d = (int *)r->data;
        *d += 1;
        distruggi_entry(e);
    }
}

int conta(char *s)
{
    ENTRY *e = crea_entry(s, 1);
    ENTRY *r = hsearch(*e, FIND);
    int n = 0;
    if (r != NULL)
    {
        n = *((int *)r->data);
    }
    distruggi_entry(e);
    return n;
}

void Distruggi_lista(Node *head)
{
    Node *cur = head;
    while (cur != NULL)
    {
        Node *tmp = cur;
        cur = (Node *)tmp->next;
        distruggi_entry(tmp->e);
        free(tmp);
    }
}

void *CapoScrittore(void *arg)
{
    Cargs *a = (Cargs *)arg;
    int fd = open("caposc", O_RDWR);
    ssize_t bytes_letti;
    char length[4];
    while ((bytes_letti = read(fd, length, sizeof(length))) > 0)
    {
        int l = atoi(length);
        if (l == 0)
        {
            break;
        }
        char riga[l + 1];
        if (read(fd, riga, l) != l)
        {
            termina("Errore nella lettura dalla named pipe");
        }
        riga[l] = '\0';
        char *token;
        char *save;
        token = strtok_r(riga, ".,:; \n\r\t", &save);
        while (token != NULL)
        {
            pthread_mutex_lock(a->bf->mutex);
            while (a->bf->counter == PC_buffer_len)
            {
                pthread_cond_wait(a->bf->empty, a->bf->mutex);
            }
            char **tmp_bf = (char **)(a->bf->buffer);
            tmp_bf[a->bf->in++ % PC_buffer_len] = token;
            a->bf->counter++;
            pthread_mutex_unlock(a->bf->mutex);
            pthread_cond_signal(a->bf->full);
            token = strtok_r(NULL, ".,:; \n\r\t", &save);
        }
        free(token);
        sleep(1);
    }
    close(fd);
    return NULL;
}

void *CapoLettore(void *arg)
{
    Cargs *a = (Cargs *)arg;
    int fd = open("capolet", O_RDWR);
    ssize_t bytes_letti;
    char length[4];
    while ((bytes_letti = read(fd, length, sizeof(length))) > 0)
    {
        int l = atoi(length);
        if (l == 0)
        {
            break;
        }
        char riga[l + 1];
        if (read(fd, riga, l) != l)
        {
            termina("Errore nella lettura dalla named pipe");
        }
        riga[l] = '\0';
        char *token;
        char *save;
        token = strtok_r(riga, ".,:; \n\r\t", &save);
        while (token != NULL)
        {
            pthread_mutex_lock(a->bf->mutex);
            while (a->bf->counter == PC_buffer_len)
            {
                pthread_cond_wait(a->bf->empty, a->bf->mutex);
            }
            a->bf->buffer[a->bf->in++ % PC_buffer_len] = token;
            a->bf->counter++;
            pthread_mutex_unlock(a->bf->mutex);
            pthread_cond_signal(a->bf->full);
            token = strtok_r(NULL, ".,:; \n\r\t", &save);
        }
        free(token);
        sleep(1);
    }
    close(fd);
    return NULL;
}

void *Gestore(void *arg)
{
    Gargs *a = (Gargs *)arg;
    int sig;
    ssize_t tmp;
    char *term = "0000";
    while (1)
    {
        sigwait(a->set, &sig);
        if (sig == SIGINT)
        {
            fprintf(stderr, "Nella tabella hash ci sono %d stringhe distinte.\n", tot);
        }
        else if (sig == SIGTERM)
        {
            int fds = open("caposc", O_RDWR);
            tmp = write(fds, term, strlen(term));
            printf("tmp: %ld\n", tmp);
            if (tmp != strlen(term))
            {
                termina("Errore nella scrittura di term sulla named pipe caposc");
            }
            close(fds);
            int fdl = open("capolet", O_RDWR);
            tmp = write(fdl, term, strlen(term));
            printf("tmp: %ld\n", tmp);
            if (tmp != strlen(term))
            {
                termina("Errore nella scrittura di term sulla named pipe capolet");
            }
            close(fdl);
            pthread_join(*(a->capoS), NULL);
            pthread_join(*(a->capoL), NULL);
            *(a->fine) = 1;
            pthread_cond_broadcast(a->S_full);
            for (int i = 0; i < (a->w); i++)
            {
                pthread_join(a->Scrittori[i], NULL);
            }
            pthread_cond_broadcast(a->L_full);
            for (int i = 0; i < (a->r); i++)
            {
                pthread_join(a->Lettori[i], NULL);
            }

            printf("Nella tabella hash ci sono %d stringhe distinte.\n", tot);
            fflush(stdout);
            Distruggi_lista(head);
            hdestroy();
            break;
        }
        else if(sig == SIGUSR1)
        {
            pthread_mutex_lock(a->ht_mutex);
            Distruggi_lista(head);
            hdestroy();
            hcreate(Num_elem);
            pthread_mutex_unlock(a->ht_mutex);
        }
    }
    return NULL;
}

void *Scrittore(void *arg)
{
    SLargs *a = (SLargs *)arg;
    while (1)
    {
        pthread_mutex_lock(a->bf->mutex);
        while (a->bf->counter == 0)
        {
            if (*(a->fine))
            {
                break;
            }
            pthread_cond_wait(a->bf->full, a->bf->mutex);
        }
        if (a->bf->counter == 0 && *(a->fine))
        {
            pthread_mutex_unlock(a->bf->mutex);
            break;
        }
        char *token = a->bf->buffer[a->bf->out++ % PC_buffer_len];
        a->bf->counter--;
        pthread_mutex_unlock(a->bf->mutex);
        pthread_cond_signal(a->bf->empty);
        pthread_mutex_lock(a->hmutex);
        aggiungi(token);
        pthread_mutex_unlock(a->hmutex);
    }
    return NULL;
}

void *Lettore(void *arg)
{
    SLargs *a = (SLargs *)arg;
    while (1)
    {
        pthread_mutex_lock(a->bf->mutex);
        while (a->bf->counter == 0)
        {
            if (*(a->fine))
            {
                break;
            }
            pthread_cond_wait(a->bf->full, a->bf->mutex);
        }
        if (a->bf->counter == 0 && *(a->fine))
        {
            pthread_mutex_unlock(a->bf->mutex);
            break;
        }
        char *token = a->bf->buffer[a->bf->out++ % PC_buffer_len];
        a->bf->counter--;
        pthread_mutex_unlock(a->bf->mutex);
        pthread_cond_signal(a->bf->empty);
        pthread_mutex_lock(a->hmutex);
        int tmp = conta(token);
        pthread_mutex_unlock(a->hmutex);
        pthread_mutex_lock(a->fmutex);
        FILE *f = fopen("lettori.log", "a");
        if (f == NULL)
        {
            termina("Errore fopen");
        }
        fprintf(f, "%s %d\n", token, tmp);
        fflush(f);
        fclose(f);
        pthread_mutex_unlock(a->fmutex);
    }
    return NULL;
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        termina("Uso: ./archivio <w> <r>\n");
    }

    int ht = hcreate(Num_elem);

    if (ht == 0)
    {
        termina("Errore creazione HT");
    }

    int w = atoi(argv[1]);
    int r = atoi(argv[2]);
    char *bufS[PC_buffer_len];
    char *bufL[PC_buffer_len];
    int fine = 0;
    
    Buffer bsc;
    Buffer blet;
    
    for (int i = 0; i < PC_buffer_len; i++)
    {
        bufS[i] = "";
        bsc.buffer[i] = bufS[i];
        bufL[i] = "";
        blet.buffer[i] = bufL[i];
    }

    pthread_mutex_t f_mutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_t ht_mutex = PTHREAD_MUTEX_INITIALIZER;
    
    // BUFFER SCRITTORI e CAPOSCRITTORI
    pthread_mutex_t sc_mutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_t sc_empty = PTHREAD_COND_INITIALIZER;
    pthread_cond_t sc_full = PTHREAD_COND_INITIALIZER;
    bsc.in = 0;
    bsc.out = 0;
    bsc.counter = 0;
    bsc.mutex = &sc_mutex;
    bsc.full = &sc_empty;
    bsc.empty = &sc_full;

    // BUFFER LETTORI e CAPOLETTORI    
    pthread_mutex_t let_mutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_t let_empty = PTHREAD_COND_INITIALIZER;
    pthread_cond_t let_full = PTHREAD_COND_INITIALIZER;
    blet.in = 0;
    blet.out = 0;
    blet.counter = 0;
    blet.mutex = &let_mutex;
    blet.full = &let_empty;
    blet.empty = &let_full;
    
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGINT);
    sigaddset(&set, SIGTERM);
    sigaddset(&set, SIGUSR1);
    pthread_sigmask(SIG_BLOCK, &set, NULL);

    pthread_t Scrittori[w];
    pthread_t Lettori[r];
    pthread_t capoS;
    pthread_t capoL;
    pthread_t signal_handler;
    
    // ARGOMENTI GESTORE
    Gargs set_args;
    set_args.Scrittori = Scrittori;
    set_args.Lettori = Lettori;
    set_args.capoS = &capoS;
    set_args.capoL = &capoL;
    set_args.w = w;
    set_args.r = r;
    set_args.fine = &fine;
    set_args.set = &set;
    set_args.S_full = &sc_full;
    set_args.L_full = &let_full;
    set_args.ht_mutex = &ht_mutex;

    // ARGOMENTI CAPO SCRITTORI e CAPO LETTORI
    Cargs capoS_args;
    Cargs capoL_args;
    capoS_args.bf = &bsc;
    capoL_args.bf = &blet;

    pthread_create(&capoS, NULL, CapoScrittore, &capoS_args);
    pthread_create(&capoL, NULL, CapoLettore, &capoL_args);
    pthread_create(&signal_handler, NULL, Gestore,&set_args);

    // ARGOMENTI SCRITTORI e LETTORI
    SLargs argS;
    SLargs argL;
    argS.bf = &bsc;
    argS.fine = &fine;
    argL.bf = &blet;
    argL.fine = &fine;

    for (int i = 0; i < w; i++)
    {
        pthread_create(&Scrittori[i], NULL, Scrittore, &argS);
    }
    for (int i = 0; i < r; i++)
    {
        pthread_create(&Lettori[i], NULL, Lettore, &argL);
    }
    
    pthread_join(signal_handler, NULL);

    // DISTRUGGO TUTTI I MUTEX E LE COND
    pthread_mutex_destroy(&sc_mutex);
    pthread_cond_destroy(&sc_full);
    pthread_cond_destroy(&sc_empty);
    pthread_mutex_destroy(&let_mutex);
    pthread_cond_destroy(&let_full);
    pthread_cond_destroy(&let_empty);
    pthread_mutex_destroy(&ht_mutex);
    pthread_mutex_destroy(&f_mutex);
    return 0;
}
