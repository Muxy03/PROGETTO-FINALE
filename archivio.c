#include "progetto.h"

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        termina("Uso: ./archivio <w> <r>\n");
    }

    int ht = hcreate(Num_elem);
    if (ht == 0)
    {
        termina("Errore creazione HT");
    }

    int w = atoi(argv[1]);
    int r = atoi(argv[2]);

    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGINT);
    sigaddset(&set, SIGTERM);
    sigaddset(&set, SIGUSR1);
    pthread_sigmask(SIG_BLOCK, &set, NULL);
    int fine = 0;
    pthread_mutex_t hmutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_t fmutex = PTHREAD_MUTEX_INITIALIZER;

    pthread_t Scrittori[w];
    pthread_t Lettori[r];
    pthread_t capoL, capoS, GS;

    // BUFFER CAPOLETTORE E LETTORI
    char bufferL[PC_buffer_len][Max_sequence_length];
    int inL = 0;
    int outL = 0;
    int counterL = 0;
    pthread_mutex_t mutexL = PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_t fullL = PTHREAD_COND_INITIALIZER;
    pthread_cond_t emptyL = PTHREAD_COND_INITIALIZER;

    Buffer bfl;
    bfl.buffer = bufferL;
    bfl.in = &inL;
    bfl.out = &outL;
    bfl.counter = &counterL;
    bfl.mutex = &mutexL;
    bfl.full = &fullL;
    bfl.empty = &emptyL;

    // BUFFER CAPOSCRITTORE E SCRITTORI
    char bufferS[PC_buffer_len][Max_sequence_length];
    int inS = 0;
    int outS = 0;
    int counterS = 0;
    pthread_mutex_t mutexS = PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_t fullS = PTHREAD_COND_INITIALIZER;
    pthread_cond_t emptyS = PTHREAD_COND_INITIALIZER;

    Buffer bfs;
    bfs.buffer = bufferS;
    bfs.in = &inS;
    bfs.out = &outS;
    bfs.counter = &counterS;
    bfs.mutex = &mutexS;
    bfs.full = &fullS;
    bfs.empty = &emptyS;

    CArg a;
    a.bf = &bfl;

    CArg d;
    d.bf = &bfs;

    SLArg b;
    b.bf = &bfl;
    b.fine = &fine;
    b.hmutex = &hmutex;
    b.fmutex = &fmutex;

    SLArg e;
    e.bf = &bfs;
    e.fine = &fine;
    e.hmutex = &hmutex;

    GArg c;
    c.Scrittori = Scrittori;
    c.Lettori = Lettori;
    c.capoS = &capoS;
    c.capoL = &capoL;
    c.w = w;
    c.r = r;
    c.fine = &fine;
    c.set = &set;
    c.S_full = &fullS;
    c.L_full = &fullL;
    c.ht_mutex = &hmutex;

    pthread_create(&capoS, NULL, CapoScrittore, &d);
    pthread_create(&capoL, NULL, CapoLettore, &a);
    pthread_create(&GS, NULL, Gestore, &c);

    for (int i = 0; i < w; i++)
    {
        pthread_create(&Scrittori[i], NULL, Scrittore, &e);
    }
    for (int i = 0; i < r; i++)
    {
        pthread_create(&Lettori[i], NULL, Lettore, &b);
    }

    pthread_join(GS, NULL);

    for (int i = 0; i < w; i++)
    {
        pthread_join(Scrittori[i], NULL);
    }

    for (int i = 0; i < r; i++)
    {
        pthread_join(Lettori[i], NULL);
    }

    pthread_mutex_destroy(&mutexS);
    pthread_cond_destroy(&fullS);
    pthread_cond_destroy(&emptyS);
    pthread_mutex_destroy(&mutexL);
    pthread_cond_destroy(&fullL);
    pthread_cond_destroy(&emptyL);
    pthread_mutex_destroy(&hmutex);
    pthread_mutex_destroy(&fmutex);

    printf("Fine\n");
    return 0;
}
