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
    size_t len = 0,tmp;
    int nseq;
    char *stop = "";

    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        termina("Errore creazione socket");
    }

    if (connect(fd, &serv_addr, sizeof(serv_addr)) < 0)
    {
        termina("Errore apertura connessione");
    }

    tmp = write(fd, tipoc, sizeof(tipoc));

    while (getline(&line, &len, f) != -1)
    {
        if (strlen(line) > 0 && strlen(line) <= Max_sequence_length && strcmp(line, "\n") != 0)
        {
            printf("Linea: %s", line);
            tmp = write(fd, line, strlen(line));
            sleep(1);
        }
    }
    sleep(1);
    tmp = write(fd, stop, strlen(stop));
    fclose(f);
    tmp = read(fd, &nseq, sizeof(nseq));
    printf("Numero sequenze: %d\n", ntohl(nseq));
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
