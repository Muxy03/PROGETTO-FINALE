#include "progetto.h"

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
        {
            termina("errore malloc head");
        }
        head->e = e;
        head->next = NULL;
    }
    else
    {
        Node *tmp = malloc(sizeof(Node));
        if (tmp == NULL)
        {
            termina("errore malloc tmp");
        }
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
        if (r == NULL)
        {
            termina("errore o tabella piena");
        }
        tot++;
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
    CArg *a = (CArg *)arg;
    int fd = open("caposc", O_RDWR);
    if (fd == -1)
    {
        termina("Errore apertura FIFO");
    }
    char length[4];
    char *tmp;
    while (1)
    {
        read(fd, length, sizeof(length));

        int l = atoi(length);

        if (strcmp(length, "0000") == 0)
        {
            break;
        }

        tmp = (char *)malloc(l + 1);
        read(fd, tmp, l);

        tmp[l] = '\0';

        char *token;
        char *save;
        token = strtok_r(tmp, ".,:; \n\r\t", &save);
        while (token != NULL)
        {
            pthread_mutex_lock(a->bf->mutex);
            while (*(a->bf->counter) == PC_buffer_len)
            {
                pthread_cond_wait(a->bf->empty, a->bf->mutex);
            }
            strcpy(a->bf->buffer[*(a->bf->in) % PC_buffer_len], token);
            *(a->bf->in) += 1;
            *(a->bf->counter) += 1;
            pthread_cond_signal(a->bf->full);
            pthread_mutex_unlock(a->bf->mutex);

            token = strtok_r(NULL, ".,:; \n\r\t", &save);
        }
        free(tmp);
        sleep(1);
    }
    close(fd);
    pthread_exit(NULL);
}

void *CapoLettore(void *arg)
{
    CArg *a = (CArg *)arg;
    int fd = open("capolet", O_RDWR);
    if (fd == -1)
    {
        termina("Errore apertura FIFO");
    }
    char length[4];
    char *tmp;
    while (1)
    {
        read(fd, length, sizeof(length));

        int l = atoi(length);

        if (strcmp(length, "0000") == 0)
        {
            break;
        }

        tmp = (char *)malloc(l + 1);
        read(fd, tmp, l);

        tmp[l] = '\0';

        char *token;
        char *save;
        token = strtok_r(tmp, ".,:; \n\r\t", &save);
        while (token != NULL)
        {
            pthread_mutex_lock(a->bf->mutex);
            while (*(a->bf->counter) == PC_buffer_len)
            {
                pthread_cond_wait(a->bf->empty, a->bf->mutex);
            }
            strcpy(a->bf->buffer[*(a->bf->in) % PC_buffer_len], token);
            *(a->bf->in) += 1;
            *(a->bf->counter) += 1;
            pthread_cond_signal(a->bf->full);
            pthread_mutex_unlock(a->bf->mutex);

            token = strtok_r(NULL, ".,:; \n\r\t", &save);
        }
        free(tmp);
        sleep(1);
    }
    close(fd);
    pthread_exit(NULL);
}

void *Gestore(void *arg)
{
    GArg *a = (GArg *)arg;
    int sig;
    while (1)
    {
        sigwait(a->set, &sig);
        if (sig == SIGINT)
        {
            fprintf(stderr, "\nNella tabella hash ci sono %d stringhe distinte.\n", tot);
        }
        else if (sig == SIGTERM)
        {
            int fds = open("caposc", O_RDWR);
            write(fds, "0000", 4 * sizeof(char));
            close(fds);
            int fdl = open("capolet", O_RDWR);
            write(fdl, "0000", 4 * sizeof(char));
            close(fdl);
            pthread_join(*(a->capoS), NULL);
            pthread_join(*(a->capoL), NULL);
            *(a->fine) = 1;

            pthread_cond_broadcast(a->S_full);
            pthread_cond_broadcast(a->L_full);

            printf("\nNella tabella hash ci sono %d stringhe distinte.\n", tot);
            fflush(stdout);
            Distruggi_lista(head);
            hdestroy();
            tot = 0;
            break;
        }
        else if (sig == SIGUSR1)
        {
            pthread_mutex_lock(a->ht_mutex);
            Distruggi_lista(head);
            hdestroy();
            hcreate(Num_elem);
            tot = 0;
            printf("Tabella hash resettata - %d\n", tot);
            pthread_mutex_unlock(a->ht_mutex);
        }
    }
    pthread_exit(NULL);
}

void *Scrittore(void *arg)
{
    SLArg *a = (SLArg *)arg;
    char *token;
    while (1)
    {
        pthread_mutex_lock(a->bf->mutex);
        while (*(a->bf->counter) == 0 && *(a->fine) == 0)
        {
            pthread_cond_wait(a->bf->full, a->bf->mutex);
        }

        if (*(a->fine) == 1)
        {
            pthread_mutex_unlock(a->bf->mutex);
            break;
        }

        token = a->bf->buffer[*(a->bf->out) % PC_buffer_len];
        *(a->bf->out) += 1;
        *(a->bf->counter) -= 1;
        pthread_mutex_unlock(a->bf->mutex);
        pthread_cond_signal(a->bf->empty);

        pthread_mutex_lock(a->hmutex);

        aggiungi(token);

        pthread_mutex_unlock(a->hmutex);
    }
    pthread_exit(NULL);
}

void *Lettore(void *arg)
{
    SLArg *a = (SLArg *)arg;
    char *token = NULL;
    int tmp = 0;
    while (1)
    {
        pthread_mutex_lock(a->bf->mutex);
        while (*(a->bf->counter) == 0 && *(a->fine) == 0)
        {
            pthread_cond_wait(a->bf->full, a->bf->mutex);
        }

        if (*(a->fine) == 1)
        {
            pthread_mutex_unlock(a->bf->mutex);
            break;
        }

        token = a->bf->buffer[*(a->bf->out) % PC_buffer_len];
        *(a->bf->out) += 1;
        *(a->bf->counter) -= 1;
        pthread_mutex_unlock(a->bf->mutex);
        pthread_cond_signal(a->bf->empty);
        pthread_mutex_lock(a->hmutex);

        tmp = conta(token);

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
    pthread_exit(NULL);
}
