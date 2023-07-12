#include "progetto.h"

#define tipoc "0"

int main(int argc, char const *argv[])
{
    if (argc != 2)
    {
        termina("Uso ./client1 nomefile");
    }

    int fd = 0;
    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    serv_addr.sin_addr.s_addr = inet_addr(HOST);

    char *line = NULL;
    size_t len = 0;
    ssize_t size = 0;

    FILE *f = fopen(argv[1], "r");

    while (getline(&line, &len, f) != -1)
    {
        if (strlen(line) > 0 && strlen(line) <= Max_sequence_length && isspace(line[0]) == 0)
        {
            if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
            {
                termina("Errore creazione socket");
            }

            if (connect(fd, &serv_addr, sizeof(serv_addr)) < 0)
            {
                termina("Errore apertura connessione");
            }

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
