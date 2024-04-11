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
    pthread_mutex_t mutex;

    pthread_cond_t ab, r;
    int blocked_ab, blocked_r;

    int active_ab, active_r;
} gestore;

void initGestore(struct gestore_t *g)
{
    pthread_mutexattr_t mutex;
    pthread_condattr_t cond;

    pthread_mutexattr_init(&mutex);
    pthread_condattr_init(&cond);

    pthread_mutex_init(&g->mutex, &mutex);
    pthread_cond_init(&g->ab, &cond);
    pthread_cond_init(&g->r, &cond);

    pthread_mutexattr_destroy(&mutex);
    pthread_condattr_init(&cond);

    g->blocked_ab = g->blocked_r = 0;
    g->active_ab = 0;
    g->active_r = 0;
}

// iniziamo dalla procedura A: quando mi blocco. --> Se c'è un reset in esecuzione.

void StartProcA(struct gestore_t *g)
{
    pthread_mutex_lock(&g->mutex);

    while (g->active_r || g->blocked_r)
    {
        // mi blocco
        g->blocked_ab++;
        pthread_cond_wait(&g->ab, &g->mutex);
        g->blocked_ab--;
    }
    g->active_ab++;
    pthread_mutex_unlock(&g->mutex);
}
// CORRETTA

void EndProcA(struct gestore_t *g)
{
    pthread_mutex_lock(&g->mutex);
    g->active_ab--;
    if (g->active_ab == 0 && g->blocked_r)
    {
        // sono l'ultimo della mia classe. devo svegliare reset
        pthread_cond_signal(&g->r);
    }
    pthread_mutex_unlock(&g->mutex);
}

void StartProcB(struct gestore_t *g)
{
    StartProcA(g);
}

void EndProcB(struct gestore_t *g)
{
    EndProcA(g);
}

// CORRETTO
void StartReset(struct gestore_t *g)
{
    pthread_mutex_lock(&g->mutex);

    while (g->active_ab != 0)
    {
        // mi blocco
        g->blocked_r++;
        pthread_cond_wait(&g->r, &g->mutex);
        g->blocked_r--;
    }
    g->active_r++;
    pthread_mutex_unlock(&g->mutex);
}

void EndReset(struct gestore_t *g)
{
    pthread_mutex_lock(&g->mutex);

    g->active_r--;

    if (g->blocked_ab)
    {
        pthread_cond_broadcast(&g->ab);
        /* code */
    }

    pthread_mutex_unlock(&g->mutex);
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
    pthread_create(&p, &a, PB, NULL);
    pthread_create(&p, &a, PR, NULL);

    pthread_attr_destroy(&a);

    /* aspetto 10 secondi prima di terminare tutti quanti */
    sleep(5);

    return 0;
}