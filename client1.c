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

// host e port a cui connettersi
#define HOST "127.0.0.1"
#define PORT 57943 // MAT:637943
#define Max_sequence_length 2048
#define typec "0"

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

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        termina("Uso ./client1 nome-file\n");
    }

    char *line;
    size_t len;
    ssize_t e;
    FILE *f = fopen(argv[1], "r");

    int fd_skt = 0;
    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    serv_addr.sin_addr.s_addr = inet_addr(HOST);

    while (getline(&line, &len, f) > 0)
    {

        if ((fd_skt = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        {
            termina("Errore creazione socket");
        }

        printf("creato socket\n");

        if (connect(fd_skt, &serv_addr, sizeof(serv_addr)) < 0)
        {
            termina("Errore apertura connessione");
        }

        printf("aperta connessione\n");

        e = write(fd_skt, typec, sizeof(typec));

        printf("scritto tipo\n");

        if (e < 0)
        {
            termina("Errore scrittura tipo");
        }

        if (strlen(line) > 0 && strlen(line) <= Max_sequence_length)
        {
            printf("scritto linea:%s\n", line);
            // size_t size = strlen(line); // 32 bit -> 4 bytes
            // e = write(fd_skt, (void *)size, sizeof(size_t));
            e = write(fd_skt, line, strlen(line));

            if (e < 0)
            {
                termina("Errore scrittura tipo");
            }
        }

        if (close(fd_skt) < 0)
        {
            termina("Errore chiusura connessione");
        }

        printf("chiusa connessione\n");

        free(line);
    }

    return 0;
}
