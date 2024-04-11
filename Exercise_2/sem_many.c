// VERSIONE CON LA MUTUA ESCLUSIONE DEI PROCESSI SOLO 1 TRA A E B PUÒ INVOCARE ALLA VOLTA O A O B.

#include <stdio.h>
#include <semaphore.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <time.h>
int nanosleep(const struct timespec *req, struct timespec *rem);
void pausetta(void)
{
    struct timespec t;
    t.tv_sec = 0;
    t.tv_nsec = (rand() % 10 + 1) * 1000000;
    nanosleep(&t, NULL);
}

struct gestore_t
{
    sem_t mutex, proc_A, proc_B;

    sem_t ab, r;
    int blocked_ab, blocked_r;

    int active_ab, active_r;
} gestore;

void initGestore(struct gestore_t *g)
{

    sem_init(&g->mutex, 0, 1);
    sem_init(&g->proc_A, 0, 1);
    sem_init(&g->proc_B, 0, 1);

    sem_init(&g->ab, 0, 0);
    sem_init(&g->r, 0, 0);

    g->blocked_ab = g->blocked_r = 0;
    g->active_ab = 0;
    g->active_r = 0;
}

// iniziamo dalla procedura A: quando mi blocco. --> Se c'è un reset in esecuzione.

// corretta
void StartProcA(struct gestore_t *g)
{
    sem_wait(&g->proc_A);
    // prendo il mutex
    sem_wait(&g->mutex);

    if (g->active_r || g->blocked_r)
    {
        // mi blocco
        g->blocked_ab++;
    }
    else
    {
        sem_post(&g->ab);
        g->active_ab++;
    }
    sem_post(&g->mutex);
    sem_wait(&g->ab);
}
// CORRETTA

void EndProcA(struct gestore_t *g)
{
    sem_wait(&g->mutex);
    g->active_ab--;
    if (g->active_ab == 0 && g->blocked_r)
    {
        // sono l'ultimo della mia classe. devo svegliare reset
        g->active_r++;
        g->blocked_r--;
        sem_post(&g->r);
    }
    sem_post(&g->proc_A);
    sem_post(&g->mutex);
}

void StartProcB(struct gestore_t *g)
{
    sem_wait(&g->proc_B);
    // prendo il mutex
    sem_wait(&g->mutex);

    if (g->active_r || g->blocked_r)
    {
        // mi blocco
        g->blocked_ab++;
    }
    else
    {
        sem_post(&g->ab);
        g->active_ab++;
    }
    sem_post(&g->mutex);
    sem_wait(&g->ab);
}

void EndProcB(struct gestore_t *g)
{

    sem_wait(&g->mutex);
    g->active_ab--;
    if (g->active_ab == 0 && g->blocked_r)
    {
        // sono l'ultimo della mia classe. devo svegliare reset
        g->active_r++;
        g->blocked_r--;
        sem_post(&g->r);
    }
    sem_post(&g->proc_B);
    sem_post(&g->mutex);
}

// CORRETTO
void StartReset(struct gestore_t *g)
{
    sem_wait(&g->mutex);
    if (!g->active_ab)
    {

        sem_post(&g->r);
        g->active_r++;
    }
    else
    {
        g->blocked_r++;
    }
    sem_post(&g->mutex);
    sem_wait(&g->r);
}

void EndReset(struct gestore_t *g)
{
    sem_wait(&g->mutex);

    g->active_r--;

    while (g->blocked_ab)
    {
        sem_post(&g->ab);
        /* code */
        g->blocked_ab--;
        g->active_ab++;
    }

    sem_post(&g->mutex);
}

/* le funzioni della risorsa R fittizia */

#define BUSY 1000000
#define CYCLE 50

void myprint(char *s)
{
    int i, j;
    fprintf(stderr, "[");
    for (j = 0; j < CYCLE; j++)
    {
        fprintf(stderr, "%s", s);
        for (i = 0; i < BUSY; i++)
            ;
    }
    fprintf(stderr, "]");
}
void ProcA(void)
{
    myprint("-");
}

void ProcB(void)
{
    myprint("+");
}

void Reset(void)
{
    myprint(".");
}

void *PA(void *arg)
{
    for (;;)
    {
        fprintf(stderr, "A");
        StartProcA(&gestore);
        ProcA();
        EndProcA(&gestore);
        fprintf(stderr, "a");
    }
    return 0;
}

void *PB(void *arg)
{
    for (;;)
    {
        fprintf(stderr, "B");
        StartProcB(&gestore);
        ProcB();
        EndProcB(&gestore);
        fprintf(stderr, "b");
    }
    return 0;
}

void *PR(void *arg)
{
    for (;;)
    {
        fprintf(stderr, "R");
        StartReset(&gestore);
        Reset();
        EndReset(&gestore);
        fprintf(stderr, "r");
        pausetta();
    }
    return 0;
}

/* ------------------------------------------------------------------------ */

int main(int argc, char **argv)
{
    pthread_attr_t a;
    pthread_t p;

    /* inizializzo il sistema */
    initGestore(&gestore);

    /* inizializzo i numeri casuali, usati nella funzione pausetta */
    srand(555);

    pthread_attr_init(&a);

    /* non ho voglia di scrivere 10000 volte join! */
    pthread_attr_setdetachstate(&a, PTHREAD_CREATE_DETACHED);

    pthread_create(&p, &a, PA, NULL);
    pthread_create(&p, &a, PA, (void *)"a");
    pthread_create(&p, &a, PA, (void *)"A");

    // pthread_create(&p, &a, PB, (void *)"B");
    // pthread_create(&p, &a, PB, (void *)"b");
    // pthread_create(&p, &a, PB, (void *)"x");

    pthread_create(&p, &a, PR, NULL);
    pthread_create(&p, &a, PR, NULL);

    pthread_attr_destroy(&a);

    /* aspetto 10 secondi prima di terminare tutti quanti */
    sleep(5);

    return 0;
}