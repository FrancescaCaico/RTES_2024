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
int nanosleep(const struct timespec *req, struct timespec *rem);

struct x_t
{
    sem_t mutex;
    sem_t threads[NTHREADS];

    int stato_risorsa[NRISORSE]; // a chi diamo cosa, se è libera o meno
    int t_waiting_ris[NTHREADS];

    int nb; // quanti sono bloccati nell'attesa di una risorsa.
} x;

int checkRisorse(int *stato)
{
    for (int i = 0; i < NRISORSE; ++i)
    {
        if (stato[i] == -1)
        {
            return i;
        }
    }
    return -1;
}

int richiesta(int t, int p)
{
    int ris = 0;
    struct timespec time;
    time.tv_nsec = 0;
    time.tv_sec = t; // proviamo a fare la semtime_wait

    sem_wait(&x.mutex);

    // quando mi blocco quando nessuna risorsa è disponibile per me --> controllo che nessuna sia disponibile
    if (checkRisorse(x.stato_risorsa) == -1)
    {
        // non c'è alcuna risorsa libera per me... faccio la wait per un certo tempo sul mio semaforo attendendo che se ne liberi qualcuna
        sem_timedwait(&x.threads[p], time.tv_sec);
    }

    sem_post(&x.mutex);

    if (ris == 0)
    {
        // è scaduto il tempo senza allocare la risorsa.
    }
}