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

// host e port a cui connettersi
#define HOST "127.0.0.1"
#define PORT 57943 // MAT:637943
#define Max_sequence_length 2048
#define typec "1"

typedef struct
{
    char *nomefile;
    pthread_mutex_t *mutex;
} args;

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
        termina("ERRORE APERTURA PIPE");
    }
    char *line = NULL;
    size_t len = 0;
    ssize_t e, letta;
    int tmp;
    char stop[1];

    int fd = 0;
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

    e = write(fd, typec, sizeof(typec));

    if (e < 0)
    {
        termina("Errore scrittura su socket");
    }

    printf("scritto tipo connessione %s\n", typec);

    while ((letta = getline(&line, &len, f)) != -1)
    {
        if (strlen(line) > 0 && strlen(line) <= Max_sequence_length)
        {
            // send(fd_skt, line, strlen(line), 0);
            e = write(fd, line, strlen(line));
        }
    }
    free(line);

    e = write(fd, stop, 0);

    e = read(fd, &tmp, sizeof(int));
    printf("letto tmp %d\n", tmp);
    printf("%ld\n", e);
    assert(e == sizeof(int));
    printf("Numero di parole: %d\n", ntohl(tmp));

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
    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_t t[nthread];
    args files[nthread];

    for (int i = 0; i < nthread; i++)
    {
        files[i].nomefile = argv[i + 1];
        files[i].mutex = &mutex;
        pthread_create(&t[i], NULL, Thread, &files[i]);
    }

    for (int i = 0; i < nthread; i++)
    {
        pthread_join(t[i], NULL);
    }

    return 0;
}