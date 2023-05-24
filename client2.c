#define _GNU_SOURCE  // permette di usare estensioni GNU
#include <stdio.h>   // permette di usare scanf printf etc ...
#include <stdlib.h>  // conversioni stringa exit() etc ...
#include <stdbool.h> // gestisce tipo bool
#include <assert.h>  // permette di usare la funzione ass
#include <string.h>  // funzioni per stringhe
#include <errno.h>   // richiesto per usare errno
#include <unistd.h>
#include <fcntl.h> /* For O_* constants */
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
#include <semaphore.h>

// host e port a cui connettersi
#define HOST "127.0.0.1"
#define PORT 57943 // MAT:637943
#define Max_sequence_length 2048
#define typec "1"

typedef struct
{
    char *nomefile;
    int *socket;
    pthread_mutex_t *semaforo;
} args;

ssize_t readn(int fd, void *ptr, size_t n)
{
    size_t nleft;
    ssize_t nread;

    nleft = n;
    while (nleft > 0)
    {
        if ((nread = read(fd, ptr, nleft)) < 0)
        {
            if (nleft == n)
                return -1; /* error, return -1 */
            else
                break; /* error, return amount read so far */
        }
        else if (nread == 0)
            break; /* EOF */
        nleft -= nread;
        ptr += nread;
    }
    return (n - nleft); /* return >= 0 */
}

ssize_t writen(int fd, void *ptr, size_t n)
{
    size_t nleft;
    ssize_t nwritten;

    nleft = n;
    while (nleft > 0)
    {
        if ((nwritten = write(fd, ptr, nleft)) < 0)
        {
            if (nleft == n)
                return -1; /* error, return -1 */
            else
                break; /* error, return amount written so far */
        }
        else if (nwritten == 0)
            break;
        nleft -= nwritten;
        ptr += nwritten;
    }
    return (n - nleft); /* return >= 0 */
}

void termina(const char *messaggio)
{
    if (errno == 0)
    {
        fprintf(stderr, "== %d == %s\n", getpid(), messaggio);
    }
    else
    {
        fprintf(stderr, "== %d == %s: %s\n", getpid(), messaggio, strerror(errno));
    }
    exit(1);
}

void *Thread(void *arg)
{
    printf("THREAD PARTITO\n");
    args *a = (args *)arg;
    FILE *f = fopen(a->nomefile, "r");
    if (f == NULL)
    {
        termina("ERRORE APERTURA file");
    }
    int fd;
    char *line = NULL;
    size_t len = 0;
    ssize_t e, letta;
    char *tmp;
    char stop = '\0';

    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    serv_addr.sin_addr.s_addr = inet_addr(HOST);

    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        termina("Errore creazione socket");
    }

    printf("creato socket\n");

    if (connect(fd, &serv_addr, sizeof(serv_addr)) < 0)
    {
        termina("Errore apertura connessione");
    }
    printf("aperta connessione\n");

    // pthread_mutex_lock(a->semaforo);
    e = write(fd, typec, sizeof(typec));
    // pthread_mutex_unlock(a->semaforo);

    if (e < 0)
    {
        termina("Errore scrittura su socket");
    }

    printf("scritto tipo connessione %s\n", typec);

    while ((letta = getline(&line, &len, f)) != -1)
    {
        if (strlen(line) > 0 && strlen(line) <= Max_sequence_length)
        {
            // pthread_mutex_lock(a->semaforo);
            printf("mando sequenza %s\n", line);
            e = write(fd, line, strlen(line));
            // pthread_mutex_unlock(a->semaforo);
        }
    }
    free(line);
    fclose(f);
    // pthread_mutex_lock(a->semaforo);
    printf("mandato stop\n");
    e = write(fd, "", 0);
    // pthread_mutex_unlock(a->semaforo);

    printf("attendendo numero di sequenze\n");
    // pthread_mutex_lock(a->semaforo);
    e = read(fd, &tmp, sizeof(tmp));
    // pthread_mutex_unlock(a->semaforo);
    printf("Numero di sequenze: %s\n", tmp);

    if (close(fd) < 0)
    {
        termina("Errore chiusura socket");
    }

    printf("chiusa connessione\n");
    printf("THREAD FINITO\n");
    return NULL;
}

int main(int argc, char *argv[])
{

    if (argc < 2)
    {
        termina("inserire almeno un file di testo dopo ./client2\n");
        return 1;
    }
    int nthread = argc - 1;
    int socket = 0;
    pthread_mutex_t semaforo = PTHREAD_MUTEX_INITIALIZER;
    pthread_t t[nthread];
    args files[nthread];

    for (int i = 0; i < nthread; i++)
    {
        files[i].nomefile = argv[i + 1];
        files[i].semaforo = &semaforo;
        files[i].socket = &i;
        pthread_create(&t[i], NULL, Thread, &files[i]);
    }

    for (int i = 0; i < nthread; i++)
    {
        pthread_join(t[i], NULL);
    }

    pthread_mutex_destroy(&semaforo);

    return 0;
}