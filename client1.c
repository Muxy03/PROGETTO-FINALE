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

int contaLinee(const char *filename){
    int lines = 0;
    char *line = NULL;
    size_t line_length = 0;
    ssize_t read;
    FILE *f = open(filename, "r");

    if (f == NULL)
    {
        termina("Impossibile aprire il file\n");
    }


    while ((read = getline(&line, &line_length, f)) != -1)
    {
        if(read <= MAX_SEQUENCE_LENGTH){
            lines++;
        }
    }
    free(line);
    if(fclose(f) != EOF){
        termina("errore chiusura file (conteggio linee)\n");
    }

    return lines;

}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        termina("Uso ./client1 nome-file\n");
    }

    char *nfile = argv[1];
    int fd_sck = 0; // file descriptor socket
    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    serv_addr.sin_addr.s_addr = inet_addr(HOST);
    int client_type = 0;
    char *line = NULL;
    size_t line_length = 0;
    ssize_t read;

    FILE *file = fopen(nfile, "r");
    if (file == NULL)
    {
        termina("Impossibile aprire il file\n");
    }

    if ((fd_sck = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        termina("Errore creazione socket\n");
    }

    if (connect(fd_sck, &serv_addr, sizeof(serv_addr)) < 0)
    {
        termina("Errore apertura connessione\n");
    }

    send(fd_sck, &client_type, sizeof(int), 0);
    while ((read = getline(&line, &line_length, file)) != -1)
    {
        assert(read <= MAX_SEQUENCE_LENGTH);
        send(fd_sck, &line, line_length, 0);
    }

    free(line);
    fclose(file);
    if (close(fd_sck) < 0)
    {
        termina("Errore chiusura socket\n");
    }

    return 0;
}
