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

typedef int T;

#define NTHREADS 4
#define NRESRC 2
#define LIBERA -1
#define TIMER 10

struct gestore_t
{
    pthread_mutex_t mutex;

    pthread_cond_t threads_waiting[NTHREADS]; // se una certa risorsa si sveglia devo necessariamente svegliare il thread che l'aspettava
    int b_on_r[NRESRC];                       // quanti sono bloccati sulle rispettive risorse in attesa della ricezione
    int resource_threads[NRESRC];             // A CHI È STATA ASSEGNATA LA RISORSA
    int threads_waiting_r[NTHREADS];          // QUALE RISORSA SI STA ASPETTANDO
    T secs[NTHREADS];                         // I TIMER DI OGNI THREADS DOPO IL QUALE VIENE COMUNQUE RISVEGLIATO SE NON LA POSSIEDE

} gestore;

void initGestore(struct gestore_t *g)
{
    pthread_mutexattr_t mutex_attr;
    pthread_condattr_t cond_attr;

    pthread_mutexattr_init(&mutex_attr);
    pthread_condattr_init(&cond_attr);

    // inizializzazione.
    pthread_mutex_init(&g->mutex, &mutex_attr);

    for (int i = 0; i < NTHREADS; ++i)
    {
        pthread_cond_init(&g->threads_waiting[i], &cond_attr);
        g->threads_waiting_r[NTHREADS] = -1; // non si aspetta alcuna risorsa inizialmente
        g->secs[i] = 0;                      // il timer è messo a 0 per tutti
    }

    for (int i = 0; i < NRESRC; ++i)
    {
        g->resource_threads[i] = -1; // inizialmente nessuno ha le risorse
        g->b_on_r[i] = 0;            // e nessuno è in attesa di risorse
    }

    pthread_mutexattr_destroy(&mutex_attr);
    pthread_condattr_destroy(&cond_attr);
}

void richiestaRisorsa(struct gestore_t *gestore, int n_r, int index_t, T sec)
{
    // ...

    /* PRENDO IL MUTEX PER ACCEDERE ALLE VARIABILI CONDIVISE*/
    pthread_mutex_lock(&gestore->mutex);
    gestore->secs[index_t] = sec;
    // fino a quando mi blocco? Fino a che mi scade il timer oppure finchè non mi viene data la risorsa che mi serve.
    printf("%d>Sta richiedendo la risorsa %d\n", index_t, n_r);

    while (gestore->secs[index_t] != 0 && gestore->resource_threads[n_r] != index_t)
    {
        gestore->b_on_r[n_r]++;
        gestore->threads_waiting_r[index_t] = n_r;
        printf("%d> Si blocca in attesa della risorsa %d\n", index_t, n_r); // Aggiunta di una stampa di debug
        pthread_cond_wait(&gestore->threads_waiting[index_t], &gestore->mutex);
        gestore->b_on_r[n_r]--;                   // mi hanno svegliato, non sono più bloccato
        gestore->threads_waiting_r[index_t] = -1; // metto a -1 la risorsa
    }
    printf("%d> Ha concluso la richiesta della risorsa --> %d\n", index_t, n_r);

    // se sono uscito mi è stata data la risorsa oppure mi è scaduto il timer...
    pthread_mutex_unlock(&gestore->mutex);
}

void rilasciaRisorsa(struct gestore_t *gestore, int n_r, int index_t)
{
    // ...

    pthread_mutex_lock(&gestore->mutex);
    printf("%d>Sta rilasciando la risorsa %d\n", index_t, n_r);

    gestore->resource_threads[n_r] = -1;      // la libero
    gestore->threads_waiting_r[index_t] = -1; // l'ho usata e non l'attendo piu

    // devo svegliare qualcuno che è in attesa della risorsa che ho appena rilasciato.
    if (gestore->b_on_r[n_r])
    {
        // c'è qualcuno bloccato, lo sveglio
        int i = 0;
        while (gestore->resource_threads[n_r] == -1 && i < NTHREADS)
        {

            // se un processo è in attesa di questa risorsa lo sveglio
            if (gestore->threads_waiting_r[i] == n_r)
            {
                gestore->b_on_r[n_r]--;
                printf("Sto assegnando la risorsa %d al processo %d\n", n_r, i);
                gestore->resource_threads[n_r] = i;
                gestore->secs[i] = 0;
                printf("%d> Sveglia il processo %d in attesa della risorsa %d\n", index_t, i, n_r); // Aggiunta di una stampa di debug
                pthread_cond_signal(&gestore->threads_waiting[i]);
            }
            ++i;
        }
    }
    printf("%d>Ha rilasciato la risorsa %d\n", index_t, n_r);

    pthread_mutex_unlock(&gestore->mutex);
}

void *scorriTempo()
{
    struct timespec t;
    int i;
    while (1)
    {
        pthread_mutex_lock(&gestore.mutex);
        for (i = 0; i < NTHREADS; i++)
        {
            if (gestore.secs[i] > 0)
            {
                gestore.secs[i]--;
            }
            else if (gestore.threads_waiting_r[i] != -1) // non mi è stata assegnata la risorsa
            {
                // mi tolgo dai bloccati
                gestore.b_on_r[gestore.threads_waiting_r[i]]--;
                gestore.threads_waiting_r[i] = -1;
                // mi sveglio
                printf("Gestore> Sveglia il processo %d in attesa della risorsa scaduta\n", i); // Aggiunta di una stampa di debug
                pthread_cond_signal(&gestore.threads_waiting[i]);
            }
        }
        pthread_mutex_unlock(&gestore.mutex);
        t.tv_sec = 1;
        t.tv_nsec = 0;
        nanosleep(&t, NULL);
    }
}
void Esegui(struct gestore_t *g, int index_t, int n_r)
{
    struct timespec t;
    t.tv_sec = (rand() % TIMER) + 1;
    t.tv_nsec = 0;
    // mi è stata assegnata la risorsa

    if (g->resource_threads[n_r] == index_t)
        printf("Processo %d: eseguo su risorsa %d per %ld secondi (risorsa 0 =%d,risorsa 1 = % d)\n ", index_t, n_r, t.tv_sec, g->resource_threads[0], g->resource_threads[1]);
    else
        printf("Processo %d: eseguo senza risorsa %d per %ld secondi (risorsa 0 = %d, risorsa 1 = %d)\n ", index_t, n_r, t.tv_sec, g->resource_threads[0], g->resource_threads[1]);
    nanosleep(&t, NULL);
}

void *Process(void *arg)
{
    struct timespec t;
    // deve specificare una risorsa
    while (1)
    {
        int num_risorsa = rand() % 2;
        int index_thread = (intptr_t)arg;
        // deve richiedere la risorsa...
        T time = (rand() % TIMER) + 1;

        richiestaRisorsa(&gestore, num_risorsa, index_thread, time);
        Esegui(&gestore, index_thread, num_risorsa);
        rilasciaRisorsa(&gestore, num_risorsa, index_thread);

        // pausetta
        t.tv_sec = (rand() % 3) + 1;
        t.tv_nsec = 0;
        nanosleep(&t, NULL);
    }
    return NULL;
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
    pthread_create(&p, &a, scorriTempo, NULL);
    for (i = 0; i < NTHREADS; i++)
        pthread_create(&p, &a, Process, (void *)(intptr_t)i);
    pthread_attr_destroy(&a);
    sleep(100);
    return 0;
}