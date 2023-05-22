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
    args *a = (args *)arg;
    FILE *f = fopen(a->nomefile, O_RDONLY);
    char *line = "vuoto";
    ssize_t e;
    int tmp;
    char stop[1];
    
    int fd_skt = 0;
    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    serv_addr.sin_addr.s_addr = inet_addr(HOST);

    if ((fd_skt = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        termina("Errore creazione socket");
    }

    if (connect(fd_skt, &serv_addr, sizeof(serv_addr)) < 0)
    {
        termina("Errore apertura connessione");
    }

    e = write(fd_skt, typec, strlen(typec));
    
    if(e < 0){
        termina("Errore scrittura su socket");
    }

    while (fgets(line,strlen(line),f) != NULL)
    {
        if (strlen(line) <= Max_sequence_length)
        {
            // send(fd_skt, line, strlen(line), 0);
            e = write(fd_skt, line, strlen(line));
        }
        //free(line);
    };

    e = write(fd_skt, stop, 0);

    e = read(fd_skt, &tmp, sizeof(int));
    assert(e == sizeof(int));
    printf("Numero di parole: %d\n", ntohl(tmp));

    if (close(fd_skt) < 0)
    {
        termina("Errore chiusura socket");
    }
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
    pthread_t t[nthread];
    args files[nthread];

    for (int i = 0; i < nthread; i++)
    {
        files[i].nomefile = argv[nthread - (i+1)];
        pthread_create(&t[i], NULL, &Thread, &files[i]);
    }

    for (int i = 0; i < nthread; i++)
    {
       pthread_join(t[i], NULL);
    }

    return 0;
}