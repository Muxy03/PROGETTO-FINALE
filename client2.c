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
#define PORT 57943 // MAT:637943
#define MAX_SEQUENCE_LENGTH 2048

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

typedef struct
{
    char *nomefile;
    int *fd_sck;
} argT;

void *ThreadF(void *arg)
{
    argT *a = (argT *)arg;

    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    serv_addr.sin_addr.s_addr = inet_addr(HOST);
    int client_type = 1;
    int terminator = 0;
    char *line = NULL;
    size_t line_length_tot = 0;
    size_t line_length = 0;
    ssize_t read;

    FILE *file = fopen(a->nomefile, "r");
    if (file == NULL)
    {
        termina("Impossibile aprire il file\n");
    }

    if ((a->fd_sck = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        termina("Errore creazione socket\n");
    }

    if (connect(a->fd_sck, &serv_addr, sizeof(serv_addr)) < 0)
    {
        termina("Errore apertura connessione\n");
    }

    send(a->fd_sck, &client_type, sizeof(int), 0);

    while ((read = getline(&line, &line_length, file)) != -1)
    {
        assert(read <= MAX_SEQUENCE_LENGTH);
        send(a->fd_sck, &line, line_length, 0);
    }

    send(a->fd_sck, &terminator, 0, 0);

    if (close(a->fd_sck) < 0)
    {
        termina("Errore chiusura socket\n");
    }

    free(line);
    fclose(file);

    return NULL;
}

int main(int argc, char *argv[])
{

    if (argc < 2)
    {
        termina("inserire almeno un file di testo dopo ./client2\n");
        return 1;
    }

    int ntextF = argc - 1;
    pthread_t t[ntextF];
    argT arguments[ntextF];

    for (int i = 0; i < ntextF; i++)
    {
        arguments[i].nomefile = argv[i + 1];
        arguments[i].fd_sck = 0;
        if (pthread_create(&t[i], NULL, ThreadF, &arguments[i]) != 0)
        {
            termina("Errore creazione thread\n");
        }
    }

    for(int i=0;i<ntextF;i++){
        if (pthread_join(t[i], NULL) != 0)
        {
            termina("Errore join thread\n");
        }
    }
    return 0;
}