#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>

/* SOLUZIONE SENZA LA PRECEDENZA DI CHI ESCE PERCHE' L'HO DIMENTICATO*/

#define NBARCHE 20
#define NPOSTI_LIBERI 8

int nanosleep(const struct timespec *req, struct timespec *rem);

void pausetta(void)
{
    struct timespec t;
    t.tv_sec = 0;
    t.tv_nsec = (rand() % 10 + 1) * 10000000;
    nanosleep(&t, NULL);
}

struct porto_t
{

    pthread_mutex_t mutex;
    pthread_cond_t transito;

    int b_transito, in_transito, in_sosta;

} porto;

void init_porto(struct porto_t *porto)
{

    pthread_mutexattr_t mutexattr;
    pthread_condattr_t condattr;

    pthread_mutexattr_init(&mutexattr);
    pthread_condattr_init(&condattr);

    pthread_mutex_init(&porto->mutex, &mutexattr);
    pthread_cond_init(&porto->transito, &condattr);

    porto->b_transito = porto->in_sosta = 0;
    porto->in_transito = 2; // al massimo due in transito

    pthread_mutexattr_destroy(&mutexattr);
    pthread_condattr_destroy(&condattr);
}

void entrata_richiestaporto(struct porto_t *porto)
{

    pthread_mutex_lock(&porto->mutex);
    printf("Barca n.%ld > Non posso ancora entrare nel porto e parcheggiare.\n In transito %d e il numero di posti occupati è %d\n\n", pthread_self(), 2 - porto->in_transito, porto->in_sosta);

    while (porto->in_sosta == NPOSTI_LIBERI || porto->in_transito == 0)
    {
        // non posso entrare nel porto perchè il canale è pieno e non ci sono posti
        //  così non occupo il canale inutilmente
        porto->b_transito++;
        printf("Barca n.%ld > Non posso ancora entrare nel porto e parcheggiare.\nIn transito %d e il numero di posti occupati è %d\n\n", pthread_self(), 2 - porto->in_transito, porto->in_sosta);
        pthread_cond_wait(&porto->transito, &porto->mutex);
        porto->b_transito--;
    }
    porto->in_sosta++;
    pthread_mutex_unlock(&porto->mutex);
}

void entrata_ok(struct porto_t *porto)
{
    // ho finito di transitare, mi parcheggio
    pthread_mutex_lock(&porto->mutex);
    porto->in_transito++;
    printf("Barca n.%ld > Mi sono parcheggiato. Il numero di posti occupati adesso è %d\n\n",
           pthread_self(),
           porto->in_sosta);

    if (porto->b_transito)
    {
        pthread_cond_signal(&porto->transito);
    }

    pthread_mutex_unlock(&porto->mutex);
}

void uscita_richiesta(struct porto_t *porto)
{

    pthread_mutex_lock(&porto->mutex);

    while (porto->in_transito == 0)
    {
        // non posso uscire dal porto perchè il canale è pieno
        porto->b_transito++;
        printf("Barca n.%ld > Non posso ancora uscire dal porto.\nIn transito %d\n",
               pthread_self(), 2 - porto->in_transito);
        pthread_cond_wait(&porto->transito, &porto->mutex);
        porto->b_transito++;
    }
    porto->in_transito--;
    porto->in_sosta--;
    pthread_mutex_unlock(&porto->mutex);
}

void uscita_ok(struct porto_t *porto)
{

    pthread_mutex_lock(&porto->mutex);

    porto->in_transito++;

    if (porto->b_transito)
    {

        pthread_cond_signal(&porto->transito);
    }
    pthread_mutex_unlock(&porto->mutex);
}

void *barca(void *arg)
{

    printf("Barca %ld > Richiedo di entrare nel porto\n", pthread_self());
    entrata_richiestaporto(&porto);
    printf("Barca %ld > In transito per entrare....\n", pthread_self());
    pausetta();
    entrata_ok(&porto);
    printf("Barca %ld > In sosta....\n", pthread_self());
    pausetta();
    printf("Barca %ld > Richiedo di uscire dal porto\n", pthread_self());
    uscita_richiesta(&porto);
    printf("Barca %ld > In transito per uscire....\n", pthread_self());
    pausetta();
    uscita_ok(&porto);
    printf("Barca %ld > Uscita!\n", pthread_self());
    return 0;
}

int main(void)
{
    pthread_attr_t a;
    pthread_t p;

    /* inizializzo il mio sistema */
    init_porto(&porto);

    /* inizializzo i numeri casuali, usati nella funzione pausetta */
    srand(555);

    pthread_attr_init(&a);

    /* non ho voglia di scrivere 10000 volte join! */
    pthread_attr_setdetachstate(&a, PTHREAD_CREATE_DETACHED);

    int i = 0;
    while (i < NBARCHE)
    {
        pthread_create(&p, &a, barca, NULL);
        i++;
    }

    pthread_attr_destroy(&a);
    /* aspetto 10 secondi prima di terminare tutti quanti */
    sleep(10);
    printf("%s", "Chiusura!\n");

    return 0;
}