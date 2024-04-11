/*Esercizio 6
In questo compito verrà affrontato il celebre problema dei filosofi a tavola, che può essere così
schematizzato :
Un certo numero N di filosofi siede intorno ad un tavolo circolare al cui centro c'è un piatto di spaghetti e
su cui sono disposte N forchette (in modo che ogni filosofo ne abbia una alla sua destra e una alla sua
sinistra).
Ogni filosofo si comporta nel seguente modo :
- Trascorre il suo tempo pensando e mangiando.
- Dopo una fase di riflessione passa a una di nutrizione.
- Per mangiare acquisisce prima la forchetta alla sua destra, quindi quella alla sua sinistra e mangia
usando entrambe.
- Una volta che ha finito di mangiare rimette a posto le due forchette che ha usato.
Il candidato :
- modelli le forchette come risorse condivise, associando quindi un semaforo ad ogni forchetta, ed ogni
filosofo come un thread e ne scriva quindi il relativo codice.
- modelli le fasi di pensiero e nutrizione come dei cicli for a vuoto di lunghezza definita dalla macro
DELAY.
- definisca il numero di filosofi (e quindi anche di forchette) usando la macro NUM_FILOSOFI.
- si sincronizzi con la fine di tutti i thread.
Si tenga presente che è stato dimostrato che, per evitare situazioni di deadlock, uno dei filosofi deve
invertire l'ordine di acquisizione delle forchette (quindi acquisirà prima quella alla sua sinistra e poi
quella alla sua destra).*/

#include <stdio.h>
#include <semaphore.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <time.h>
#include <stdbool.h>
#include <stdint.h>

#define NUM_FILOSOFI 5
#define DELAY 100

struct cena_t
{
    sem_t mutex;
    sem_t forchette[NUM_FILOSOFI];
    // dobbiamo decisamente vedere se ci sono filosofi bloccati
    int blocked[NUM_FILOSOFI];
    int active[NUM_FILOSOFI];

} cena;

void initCena(struct cena_t *cena)
{
    sem_init(&cena->mutex, 0, 1); //

    for (int i = 0; i < NUM_FILOSOFI; i++)
    {
        sem_init(&cena->forchette[i], 0, 1); //
        cena->blocked[i] = 0;
        cena->active[i] = 0;
    }
}

void acquisisciForchette(bool left_right, int index_filosofo)
{

    /*mi blocco se una delle due forchette o entrambe alla mia portata sono già assegnate ad altri filosofi.*/
    if (left_right)
    {
        // prendo prima quella a sx: ci provo altrimenti mi blocco. "Quella avanti al filosofo è la sx"
        printf("Filosofo %d: Cerco di acquisire la forchetta a sinistra...\n", index_filosofo);
        sem_wait(&cena.forchette[index_filosofo]);
        printf("Filosofo %d: Ho acquisito la forchetta a sinistra!\n", index_filosofo);
        // se ho preso la forchetta sono qui, altrimenti resto bloccato finchè il filosofo a me adiacente non la molla a questo punto provo a prendere quella a dx.

        printf("Filosofo %d: Cerco di acquisire la forchetta a destra...\n", index_filosofo);
        sem_wait(&cena.forchette[(index_filosofo + 1) % NUM_FILOSOFI]);

        printf("Filosofo %d: Ho acquisito la forchetta a destra!\n", index_filosofo);
        // se ci sono riuscito posso finalmente mangiare.
    }
    else
    {
        printf("Filosofo %d: Cerco di acquisire la forchetta a destra...\n", index_filosofo);
        // prendo prima quella a dx
        sem_wait(&cena.forchette[(index_filosofo + 1) % NUM_FILOSOFI]);
        printf("Filosofo %d: Ho acquisito la forchetta a destra!\n", index_filosofo);
        // se ho preso la forchetta sono qui, altrimenti resto bloccato finchè il filosofo a me adiacente non la molla a questo punto provo a prendere quella a sx.
        printf("Filosofo %d: Cerco di acquisire la forchetta a sinistra...\n", index_filosofo);
        sem_wait(&cena.forchette[index_filosofo]);
        printf("Filosofo %d: Ho acquisito la forchetta a sinistra!\n", index_filosofo);
    }
}

void rilasciaForchette(int index_filosofo)
{
    // sem_wait(&cena.mutex);
    sem_post(&cena.forchette[index_filosofo]);
    printf("Filosofo %d: Ho rilasciato la forchetta a sinistra!\n", index_filosofo);
    sem_post(&cena.forchette[(index_filosofo + 1) % NUM_FILOSOFI]);
    printf("Filosofo %d: Ho rilasciato la forchetta a destra!\n", index_filosofo);
    // sem_post(&cena.mutex);
}

void pensa()
{
    for (int i = 0; i < DELAY; ++i)
    {
    }
}

void mangia()
{
    pensa();
}

void *Alternate(void *arg)
{
    int index = (int)(intptr_t)arg;
    bool left = true;

    while (1)
    {
        // partiamo prendendo la sinistra
        sem_wait(&cena.mutex);
        acquisisciForchette(left, index);
        printf("Filosofo %d: posso mangiare!\n", index);
        mangia();
        rilasciaForchette(index);
        sem_post(&cena.mutex);

        printf("Filosofo %d: posso pensare!\n", index);

        pensa();
        left = !left;
    }
}

int main(void)
{

    srand(time(NULL));

    pthread_attr_t a;
    pthread_t threads[NUM_FILOSOFI];

    pthread_attr_init(&a);

    initCena(&cena);

    for (int i = 0; i < NUM_FILOSOFI; i++)
    {
        pthread_create(&threads[i], &a, Alternate, (void *)(intptr_t)i);
    }

    sleep(5);

    return 0;
}
