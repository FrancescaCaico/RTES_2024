/*
Esercizio 10
In un sistema organizzato secondo il modello a memoria comune due risorse RA e RB sono allocate
dinamicamente ai processi P1, P2, …., Pn tramite un comune gestore G. Ogni processo può richiedere al
gestore l’uso esclusivo della risorsa RA (tramite la funzione RicA) o della risorsa RB (tramite la funzione
RicB) oppure l’uso esclusivo di una qualunque delle due (tramite la funzione RicQ). Un processo può
però richiedere anche l’uso esclusivo di entrambe le risorse (tramite la funzione Ric2). Chiaramente,
dopo che la risorsa (o le risorse) è stata (sono state) utilizzata (utilizzate), il processo la (le) rilascia al
gestore tramite le opportune funzioni di rilascio.
Utilizzando il meccanismo semaforico, realizzare il gestore G con tutte le funzioni di richiesta e di
rilascio necessarie per implementare quanto sopra specificato. Nel realizzare il gestore si tenga conto
delle seguenti regole di priorità: vengono privilegiate le richieste generiche rispetto alle richieste della
risorsa RA e queste nei confronti delle richieste della risorsa RB e infine queste nei confronti delle
richieste di entrambe le risorse. Durante l’allocazione di una risorsa, in seguito ad una richiesta generica,
se sono disponibili entrambe le risorse, allocare per prima la risorsa RA.*/

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

void pausetta(void)
{
    struct timespec t;
    t.tv_sec = 0;
    t.tv_nsec = (rand() % 10 + 1) * 1000000;
    nanosleep(&t, NULL);
}
// DA IMPLEMENTARE --> RicA, releaseA, RicB, releaseB,RicQ, Ric2;
/*
    1. RAB
    2. RA
    3. RB
    4 RA_o_B
*/

// per capire il tipo di richiesta, di quale risorsa usiamo delle macro, questi valori verranno scelti casualmente all'atto della richiesta del processo.

#define richiedeA 0
#define richiedeB 1
#define richiedeA_o_B 3
#define richiedeA_e_B 2

struct gestore_t
{

    // serve un mutex per accedere alle variabili condivise.
    sem_t mutex;

    // serve un semaforo per accodarsi per una certa richiesta. Bastano 2 semafori uno per A e B, se uno le richiede entrambe le acquisisce solo se la wait ha successo per entrambi i semafori.
    sem_t semA, semB;

    // un contatore per tenere traccia di quanti sono in attesa di avere la risorsa desiderata
    int bA, bB; // int bloccati su A e B.

    // come capiamo se le risorse sono libere? Bisogna definirle.
    //  Usiamo un int che conterrà per ogni risorsa il numero del thread che la possiede in un certo momento.
    // Se nessuno la possiede conterrà -1 = RISORSA LIBERA.
    int risorsaA;
    int risorsaB;

} gestore;

void initGestore(struct gestore_t *g)
{
    sem_init(&g->mutex, 0, 1);
    sem_init(&g->semA, 0, 0);
    sem_init(&g->semB, 0, 0);

    g->bA = g->bB = 0;
    g->risorsaA = g->risorsaB = -1; // inizialmente libere.
}

void mandaEsecuzione()
{
    // Simulazione dell'esecuzione del processo
    printf("Il processo sta eseguendo...\n");
    pausetta();
    printf("Esecuzione completata.\n");
}

void RicA(struct gestore_t *g, int index_t)
{
    // richiede A
    sem_wait(&g->mutex);

    // mi blocco se A non è disponibile
    if (g->risorsaA == -1)
    {
        // la risorsa è libera --> eseguo la post previa per prendere il possesso successivamente
        printf("Processo %d >> la risorsa A è libera :) e me la prendo\n", index_t);
        g->risorsaA = index_t; // la risorsa mi appartiene
        sem_post(&g->semA);
    }
    else
    {
        // mi blocco sul semaforo e devo incrementare che c'è un bloccato in più
        printf("Processo %d >> Mi sto bloccando perchè la risorsa A non è disponibile\n", index_t);
        g->bA++;
    }
    sem_post(&g->mutex);
    sem_wait(&g->semA);
    printf("Processo %d >> Mi è stata data la risorsa A\n", index_t);
}

void RicB(struct gestore_t *g, int index_t)
{
    // richiede A
    sem_wait(&g->mutex);

    // mi blocco se A non è disponibile
    if (g->risorsaB == -1)
    {
        // la risorsa è libera --> eseguo la post previa per prendere il possesso successivamente
        printf("Processo %d >> la risorsa B è libera :) e me la prendo\n", index_t);
        g->risorsaB = index_t; // la risorsa mi appartiene
        sem_post(&g->semB);
    }
    else
    {
        // mi blocco sul semaforo e devo incrementare che c'è un bloccato in più
        printf("Processo %d >> Mi sto bloccando perchè la risorsa B non è disponibile\n", index_t);
        g->bB++;
    }
    sem_post(&g->mutex);
    sem_wait(&g->semB);
    printf("Processo %d >> Mi è stata data la risorsa B\n", index_t);
}

// // Viene chiamato il gestore.
// /*
//     1. RAB
//     2. RA
//     3. RB
//     4 RA_o_B
// */

void releaseA(struct gestore_t *g)
{
    sem_wait(&g->mutex);

    // resetto la risorsa.
    g->risorsaA = -1; // adesso è libera.

    // chi sveglio? Mi devo occupare di svegliare chi è in attesa di A dato che l'ho liberata.
    if (g->bA)
    {
        g->bA--;
        sem_post(&g->semA);
    }

    sem_post(&g->mutex);
}

void releaseB(struct gestore_t *g)
{
    sem_wait(&g->mutex);

    // resetto la risorsa.
    g->risorsaB = -1; // adesso è libera.

    // chi sveglio? Mi devo occupare di svegliare chi è in attesa di A dato che l'ho liberata.
    if (g->bB)
    {
        g->bB--;
        // casualmente qualcuno verrà risvegliato.
        sem_post(&g->semB);
    }

    sem_post(&g->mutex);
}

void RicQ(struct gestore_t *g, int index, int r)
{
    if (r == 0)
    {
        RicA(g, index);
    }
    if (r == 1)
    {
        RicB(g, index);
    }
}
void *Gestore(int richiesta, int index)
{
    if (richiesta == richiedeA_e_B)
    {
        printf("Il processo %d ha richiesto A e B insieme\n", index);
        RicA(&gestore, index);
        RicB(&gestore, index);
        mandaEsecuzione();
        releaseA(&gestore);
        releaseB(&gestore);
    }
    else if (richiesta == richiedeA)
    {
        printf("Il processo %d ha richiesto A \n", index);
        RicA(&gestore, index);
        mandaEsecuzione();
        releaseA(&gestore);
    }
    else if (richiesta == richiedeB)
    {
        printf("Il processo %d ha richiesto A \n", index);

        RicB(&gestore, index);
        mandaEsecuzione();
        releaseB(&gestore);
    }
    else if (richiesta == richiedeA_o_B)
    {
        printf("Il processo %d ha richiesto A o B\n", index);
        int A_o_B = rand() % 2;

        RicQ(&gestore, index, A_o_B);
        mandaEsecuzione();
        if (A_o_B == 0)
        {
            releaseA(&gestore);
        }
        if (A_o_B == 1)
        {
            releaseB(&gestore);
        }
    }
    return NULL;
}

void *P(void *arg)
{
    int tipo_richiesta;
    int index = (intptr_t)arg;
    while (1)
    {
        tipo_richiesta = rand() % 4;
        Gestore(tipo_richiesta, index);
        // pausetta
        pausetta();
    }
    return NULL;
}

int main()
{
    // Inizializzazione del gestore
    initGestore(&gestore);

    // Creazione di thread/processi che richiedono le risorse
    pthread_t threads[5]; // Esempio con 5 thread
    pthread_attr_t a;
    srand(time(NULL));
    pthread_attr_init(&a);
    // non ho voglia di scrivere 10000 volte join!
    pthread_attr_setdetachstate(&a, PTHREAD_CREATE_DETACHED);
    for (int i = 0; i < 5; i++)
    {

        // Creazione di un thread con la richiesta specificata
        pthread_create(&threads[i], NULL, P, (void *)(intptr_t)i);
    }
    pthread_attr_destroy(&a);
    sleep(100);

    return 0;
}