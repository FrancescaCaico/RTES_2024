/*
Esercizio 8
In un sistema a memoria comune quattro processi applicativi P1, P2, P3, P4 competono per l'uso di due
risorse equivalenti. Si chiede di scrivere il codice del gestore per allocare dinamicamente le risorse ai
richiedenti tenendo conto che, all'atto della richiesta, il richiedente specifica anche un parametro
T:integer che denota un timeout (in termini di tick di orologio) scaduto il quale, se la richiesta non è stata
esaudita il processo richiedente viene comunque svegliato pur non avendo disponibile la risorsa richiesta.
identificate le procedure del gestore con i necessari parametri, scrivere il codice del gestore supponendo
che ad ogni tick di orologio vada in esecuzione il relativo processo orologio. Se necessaria, è disponibile
la primitiva di sistema PE che restituisce l'indice del processo in esecuzione. Non è specificata nessuna
politica per quanto riguarda la priorità dei processi richiedenti.
*/

#include <stdio.h>
#include <semaphore.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <time.h>
#include <stdbool.h>
#include <stdint.h>
#include <semaphore.h>
#include <time.h>

int nanosleep(const struct timespec *req, struct timespec *rem);

#define NTHREADS 4
#define NRESRC 2
#define LIBERA -1
#define TIMER 5

struct gestore_t
{
    sem_t mutex;
    sem_t processi[NTHREADS]; // ogni processo si blocca su una coda se non ha le risorse

    int risorse[NRESRC]; // a chi è assegnata la risorsa in un certo momento --> se non gli viene assegnata esegue lo stesso per t secondi
    int Timers[NTHREADS];

    // QUANTI SONO BLOCCATI SU QUALE RISORSA? Quando esco sveglio tutti gli altri processi

    int risorse_bloccate[NRESRC];
    int bloccati[NTHREADS]; // quali processi sono bloccati su quali risorse
} gestore;

void initGestore(struct gestore_t *g)
{
    sem_init(&g->mutex, 0, 1);

    for (int i = 0; i < NTHREADS; i++)
    {
        sem_init(&g->processi[i], 0, 0);
        g->Timers[i] = 0;
        g->bloccati[i] = 0;
    }

    for (int i = 0; i < NRESRC; i++)
    {
        g->risorse[i] = 0;
    }
}

void richiediRisorsa(struct gestore_t *g, int risorsa, int index_thread)
{
    sem_wait(&g->mutex);

    // Quando mi blocco? Dopo un tot
    if (g->risorse[risorsa] == LIBERA)
    {
        // la posso prendere.
        g->risorse[risorsa] = index_thread;
        sem_post(&g->processi[index_thread]);
    }
    else
    {
        // mi blocco sul semaforo incremento il numero di thread bloccati sul mio semaforo
        g->bloccati[index_thread] = risorsa;
        g->risorse_bloccate[risorsa]++; // quanti sono bloccati su questa risorsa.
    }

    sem_post(&g->mutex);
    sem_wait(&g->processi[index_thread]);
}

void liberaRisorsa(struct gestore_t *g, int risorsa, int index_thread)
{

    // chi devo svegliare?
    sem_wait(&g->mutex);
    g->risorse[risorsa] = LIBERA; // libero la risorsa
    g->bloccati[index_thread] = -1;

    // devo svegliare qualcuno che è in attesa della risorsa che ho appena rilasciato.
    if (g->risorse_bloccate[risorsa])
    {
        // c'è qualcuno bloccato, lo sveglio
        int i = 0;
        while (g->risorse[risorsa] == LIBERA && i < NTHREADS)
        {

            // se un processo è in attesa di questa risorsa lo sveglio
            if (g->bloccati[i] == risorsa)
            {

                /* code */
                g->risorse_bloccate[risorsa]--; // uno in meno bloccato
                g->bloccati[i] = LIBERA;        // non aspetta nessuna risorsa
                g->Timers[i] = 0;
                g->risorse[risorsa] = i; // assegno la risorsa a questo processo.
                sem_post(&g->processi[i]);
            }
            ++i;
        }
    }

    sem_post(&g->mutex);
}

void Esegui(struct gestore_t *g, int index_thread, int risorsa)
{
    struct timespec t;
    t.tv_sec = (rand() % TIMER) + 1;
    t.tv_nsec = 0;
    // mi è stata assegnata la risorsa
    if (g->risorse[risorsa] == index_thread)
        printf("Processo %d: eseguo su risorsa %d per %ld secondi (risorsa 0 =%d,risorsa 1 = % d)\n ", index_thread, risorsa, t.tv_sec, g->risorse[0], g->risorse[1]);
    else
        printf("Processo %d: eseguo senza risorsa %d per %ld secondi (risorsa 0 = %d, risorsa 1 = %d)\n ", index_thread, risorsa, t.tv_sec, g->risorse[0], g->risorse[1]);
    nanosleep(&t, NULL);
}

void *Process(void *arg)
{
    // deve specificare una risorsa
    while (1)
    {
        int num_risorsa = rand() % 2;
        int index_thread = (intptr_t)arg;
        // deve richiedere la risorsa...
        richiediRisorsa(&gestore, num_risorsa, index_thread);
        Esegui(&gestore, index_thread, num_risorsa);
        liberaRisorsa(&gestore, num_risorsa, index_thread);
    }
    return NULL;
}

void *orologio(void *arg)
{
    struct timespec t;
    int i;
    while (1)
    {
        sem_wait(&gestore.mutex);
        for (i = 0; i < NTHREADS; i++)
        {
            if (gestore.Timers[i] > 0)
                gestore.Timers[i]--;
            else if (gestore.bloccati[i] != -1)
            {
                // esegue senza la risorsa. lo tolgo dai bloccati sulle risorse
                gestore.risorse_bloccate[gestore.bloccati[i]]--; // uno sul quella risorsa prende la risorsa e non aspetta piu
                gestore.bloccati[i] = -1;
                sem_post(&gestore.processi[i]);
            }
        }
        sem_post(&gestore.mutex);
        t.tv_sec = 1;
        t.tv_nsec = 0;
        nanosleep(&t, NULL);
    }
}

int main(void)
{
    pthread_attr_t a;
    pthread_t p;
    int i;
    initGestore(&gestore);
    srand(time(NULL));
    pthread_attr_init(&a);
    // non ho voglia di scrivere 10000 volte join!
    pthread_attr_setdetachstate(&a, PTHREAD_CREATE_DETACHED);
    pthread_create(&p, &a, orologio, NULL);
    for (i = 0; i < NTHREADS; i++)
        pthread_create(&p, &a, Process, (void *)(intptr_t)i);
    pthread_attr_destroy(&a);
    sleep(10);
    return 0;
}