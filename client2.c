#define _GNU_SOURCE  // permette di usare estensioni GNU
#include <stdio.h>   // permette di usare scanf printf etc ...
#include <stdlib.h>  // conversioni stringa exit() etc ...
#include <stdbool.h> // gestisce tipo bool
#include <assert.h>  // permette di usare la funzione ass
#include <string.h>  // funzioni per stringhe
#include <errno.h>   // richiesto per usare errno
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>

// host e port a cui connettersi
#define HOST "127.0.0.1"
#define PORT 57943 // 637943
#define Max_sequence_length 2048
#define tipoc "1"

typedef struct
{
    char *nf;
} Targ;

void termina(const char *messaggio)
{
    if (errno == 0)
        fprintf(stderr, "== %d == %s\n", getpid(), messaggio);
    else
        fprintf(stderr, "== %d == %s: %s\n", getpid(), messaggio,
        strerror(errno));
    exit(1);
}

void *Tfunc(void *args)
{
    Targ *a = (Targ *)args;
    FILE *f = fopen(a->nf, "r");

    int fd = 0; // file descriptor associato al socket
    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    serv_addr.sin_addr.s_addr = inet_addr(HOST);

    char *line = NULL;
    size_t len = 0;
    int nseq;
    char stop = '\0';
    ssize_t e;

    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        termina("Errore creazione socket");

    if (connect(fd, &serv_addr, sizeof(serv_addr)) < 0)
        termina("Errore apertura connessione");

    e = write(fd, tipoc, sizeof(tipoc));

    while (getline(&line, &len, f) != -1)
    {
        if (strlen(line) > 0 && strlen(line) <= Max_sequence_length)
        {
            e = write(fd, line, len);
            sleep(1);
        }
    }
    sleep(1);
    e = write(fd, &stop, sizeof(stop));
    fclose(f);
    e = read(fd, &nseq, sizeof(nseq));
    printf("Numero sequenze: %d\n", ntohl(nseq));
    close(fd);
    pthread_exit(NULL);
}

int main(int argc, char const *argv[])
{
    if (argc < 2)
    {
        termina("Uso ./client2 nomefile1 nomefile2 ...");
    }

    int nthread = argc - 1;
    pthread_t t[nthread];
    Targ a[nthread];

    for (int i = 0; i < nthread; i++)
    {
        a[i].nf = argv[i + 1];
        pthread_create(&t[i], NULL, Tfunc, &a[i]);
    }

    for (int i = 0; i < nthread; i++)
    {
        pthread_join(t[i], NULL);
    }

    return 0;
}