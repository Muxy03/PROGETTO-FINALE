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

// host e port a cui connettersi
#define HOST "127.0.0.1"
#define PORT 57943 // 637943
#define Max_sequence_length 2048
#define tipoc "0"

void termina(const char *messaggio)
{
    if (errno == 0)
        fprintf(stderr, "== %d == %s\n", getpid(), messaggio);
    else
        fprintf(stderr, "== %d == %s: %s\n", getpid(), messaggio,
                strerror(errno));
    exit(1);
}

int main(int argc, char const *argv[])
{
    if (argc != 2)
    {
        termina("Uso ./client1 nomefile");
    }

    int fd = 0; // file descriptor associato al socket
    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    serv_addr.sin_addr.s_addr = inet_addr(HOST);

    char *line = NULL;
    size_t len = 0;

    FILE *f = fopen(argv[1], "r");

    while (getline(&line, &len, f) != -1)
    {
        if (strlen(line) > 0 && strlen(line) <= Max_sequence_length)
        {
            if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
                termina("Errore creazione socket");

            if (connect(fd, &serv_addr, sizeof(serv_addr)) < 0)
                termina("Errore apertura connessione");

            ssize_t size;
            size = write(fd, tipoc, sizeof(tipoc));

            size = send(fd, line, strlen(line), 0);

            close(fd);
        }
    }

    fclose(f);
    free(line);

    printf("connessione chiusa\n");
    fflush(stdout);
    return 0;
}