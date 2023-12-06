#include "progetto.h"

#define tipoc "1"

typedef struct
{
    char *nf;
} Targ;

void *Tfunc(void *args)
{
    Targ *a = (Targ *)args;
    FILE *f = fopen(a->nf, "r");

    int fd = 0;
    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    serv_addr.sin_addr.s_addr = inet_addr(HOST);

    char *line = NULL;
    size_t len = 0, tmp = 0;

    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        termina("Errore creazione socket");
    }

    if (connect(fd, &serv_addr, sizeof(serv_addr)) < 0)
    {
        termina("Errore apertura connessione");
    }

    tmp = write(fd, tipoc, 1);

    while ((tmp = getline(&line, &len, f)) != -1)
    {
        if (strlen(line) > 0 && strlen(line) <= Max_sequence_length && isspace(line[0]) == 0)
        {
            int line_len = strlen(line);

            if ((tmp = send(fd, &line_len, sizeof(int), 0)) == -1)
            {
                termina("Errore nell'invio della lunghezza della linea");
            }
            if ((tmp = send(fd, line, line_len, 0)) == -1)
            {
                termina("Errore nell'invio della linea");
            }
        }
    }

    fclose(f);
    int empty_line_len = 0;
    char *empty = "";

    if ((tmp = send(fd, &empty_line_len, sizeof(int), 0)) == -1)
    {
        termina("Errore nell'invio lunghezza della stringa vuota");
        free(line);
    }

    if ((tmp = send(fd, empty, strlen(empty), 0)) == -1)
    {
        termina("Errore nell'invio della stringa vuota");
        free(line);
    }

    int num_strings;
    if ((tmp = recv(fd, &num_strings, sizeof(int), 0)) == -1)
    {
        termina("Errore nella ricezione del numero di stringhe");
        free(line);
    }
    else
    {
        printf("Numero di stringhe ricevute dal server: %d\n", ntohl(num_strings));
        free(line);
    }

    close(fd);
    pthread_exit(NULL);
}

int main(int argc, char *argv[])
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