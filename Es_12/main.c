

#include <stdio.h>
#include <semaphore.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <stdint.h>
#include <semaphore.h>
#include <time.h>

#define NRISORSE 9
#define NTHREADS 8

/*

Esercizio 12
Un gestore di risorse equivalenti alloca dinamicamente ad N processi clienti 9 istanze di uno stesso tipo
di risorse (R1, R2, …., R9).
Gli N processi clienti (P0.,……., PN-1 ) vengono serviti dal gestore privilegiando i processi di indice più
basso.
Un processo che chiede una risorsa si blocca se all’atto della richiesta non ci sono risorse disponibili in
attesa che una risorsa venga rilasciata. Però il processo rimane bloccato, al massimo, per t quanti di
tempo successivi al blocco (dove t denota un parametro intero passato in fase di chiamata della funzione
richiesta). Quando un processo viene svegliato per lo scadere del tempo massimo da lui indicato, la
funzione richiesta termina senza allocare niente al richiedente.
Utilizzando il meccanismo semaforico realizzare il codice del gestore che offre le tre seguenti funzioni
membro:
- int richiesta (int t, int p ) dove t denota il time-out espresso come numero di quanti di tempo, mentre p
denota l’indice del processo richiedente. La funzione restituisce l’indice della
risorsa allocata (1,…, 9) oppure 0 se il processo termina la funzione dopo un
time-out senza aver allocato niente.
- void rilascio (int r) dove r denota l’indice della risorsa rilasciata.
–
void tick () funzione chiamata dal driver dell’orologio in tempo reale ad ogni interruzioni di clock.

*/

/*
 - Un processo gestore
 - N processi

*/

int risorsa_allocata;

struct x_t
{

    sem_t mutex;
    sem_t processi[NTHREADS];
    sem_t gestore;
    int risorse_disponibili[NRISORSE];
    int p_risorse[NTHREADS]; // ogni elemento ci dice se un processo è in attesa di una risorsa. ()
    int timer[NTHREADS];

    int nrichieste;
} x;

int primo_richiedente(struct x_t *x)
{

    for (int i = 0; i < NTHREADS; ++i)
    {
        if (x->p_risorse[i] == -1)
        {
            return i;
        }
    }
    return -1;
}

int prima_risorsa(struct x_t *x)
{

    for (int i = 0; i < NRISORSE; ++i)
    {
        if (x->risorse_disponibili[i])
        {
            return i;
        }
    }
    return -1;
}

void *alloca_risorsa(struct x_t *x)
{
    // il primo di indice + basso
    sem_wait(&x->mutex);
    if (x->nrichieste == 0 || x->risorse_disponibili == 0)
    {
        // nessuno ha fatto la richiesta per la risorsa oppure non ci sono risorse disponibili
        sem_post(&x->mutex);
        sem_wait(&x->gestore);
    }

    // dobbiamo accontentare il primo che ha richiesto la risorsa e di cui non è scaduto il tempo.
    int index = primo_richiedente(x);
    int prima = prima_risorsa(x);
    if (x->timer[index] && prima != -1)
    {
        x->risorse_disponibili[prima] = 0;
        x->p_risorse[index] = prima;   // al processo N ho assegnato la risorsa n.prima
        sem_post(&x->processi[index]); // sveglio il processo coinvolto.
    }
    sem_post(&x->mutex);
}

void attesa_risorsa(struct x_t *x, int t, int p)
{
}

int richiesta(int t, int p)
{
}