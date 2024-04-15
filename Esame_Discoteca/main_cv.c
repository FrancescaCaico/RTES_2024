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
/*Una discoteca ha la seguente organizzazione per far entrare i clienti nella sala da ballo.
Esiste una cassiera che emette UN BIGLIETTO PER VOLTA. Emissione del biglietto e riscossione partono assieme, ma alla fine sono non bloccanti.

- per evitare confusione alla cassa esiste una porta che fa attendere i clienti fuori dalla discoteca.
  La porta all'inizio del sistema è aperta.
  All'arrivo del primo cliente la porta si chiude dietro di lui.

- Quando l'ULTIMO cliente in coda alla cassa ha ritirato il biglietto (ovvero alla fine di cliente_esco_coda),
  se ci sono dei clienti in attesa fuori, si apre la porta, si fanno entrare (a vostra scelta se farli entrare tutti o solo parte di essi),
  dopo di che la porta si richiude di nuovo. Se non ci sono clienti in attesa fuori la porta viene aperta.

Il sistema è modellato tramite un thread per la cassiera, ed uno per ogni cliente. Il numero di clienti non è specificato. Non è importante l'ordine di accodamento dei clienti.*/

#define NTHREADS 10
int nanosleep(const struct timespec *req, struct timespec *rem);

void pausetta(void)
{
    struct timespec t;
    t.tv_sec = 0;
    t.tv_nsec = (rand() % 10 + 1) * 1000000;
    nanosleep(&t, NULL);
}

struct discoteca_t
{
    pthread_mutex_t mutex;
    pthread_cond_t fuori;    // per i thread bloccati fuori
    pthread_cond_t cassa;    // per i thread che sono dentro ma che attendono alla cassa
    pthread_cond_t cassiera; // la cassiera si blocca in attesa dei clienti

    int bfuori, bcassa;
    int ndentro; // assumiamo che in attesa alla cassa possano stare fino alla metà dei thread totali. --> discriminiamo chi aspetta fuori e chi dentro

} discoteca;

void init_discoteca(struct discoteca_t *d)
{
    pthread_mutexattr_t mutexattr;
    pthread_condattr_t condattr;

    pthread_mutexattr_init(&mutexattr);
    pthread_condattr_init(&condattr);

    pthread_mutex_init(&d->mutex, &mutexattr);
    pthread_cond_init(&d->fuori, &condattr);
    pthread_cond_init(&d->cassa, &condattr);
    pthread_cond_init(&d->cassiera, &condattr);

    pthread_mutexattr_destroy(&mutexattr);
    pthread_condattr_destroy(&condattr);

    discoteca.bfuori = discoteca.ndentro = discoteca.bcassa = 0;
}

void cliente_coda_fuori(struct discoteca_t *d)
{
    pthread_mutex_lock(&d->mutex);

    while (d->ndentro == (NTHREADS / 2 - 1))
    {
        // se è pieno dentro devo attendere fuori
        d->bfuori++;
        printf("%ld > Aspetto fuori\n", pthread_self());

        pthread_cond_wait(&d->fuori, &d->mutex);
        d->bfuori--;
    }
    printf("%ld > Sono entrato dentro\n", pthread_self());
    d->ndentro++;
    pthread_mutex_unlock(&d->mutex);
}

void cliente_coda_dentro(struct discoteca_t *d)
{
    pthread_mutex_lock(&d->mutex);

    while (d->bcassa)
    {
        d->bcassa++;
        pthread_cond_wait(&d->cassa, &d->mutex);
        d->bcassa--;
    }

    printf("%ld > E' il mio turno alla cassa\n", pthread_self());
    pthread_cond_signal(&d->cassiera);
    pthread_mutex_unlock(&d->mutex);
}

void cliente_esco_coda(struct discoteca_t *d)
{
    pthread_mutex_lock(&d->mutex);
    d->ndentro--;
    // ho pagato e ricevuto il biglietto, è mia competenza svegliare chi è in coda in cassa dietro di me
    if (d->bcassa)
    {
        d->bcassa++;
        pthread_cond_signal(&d->cassa);
    }
    else if (d->bfuori)
    {

        pthread_cond_broadcast(&d->fuori);
    }

    pthread_mutex_unlock(&d->mutex);
}

void cassiera_attesa_cliente(struct discoteca_t *d)
{
    printf("Sono la cassiera > attendo un cliente in cassa\n");
    pthread_cond_signal(&d->cassa);
    pthread_cond_wait(&d->cassiera, &d->mutex);
}

void cassiera_cliente_servito(struct discoteca_t *d)
{
    printf("Cassiera > Biglietto emesso e pagato, cliente servito.\n");
}

void *cliente(void *arg)
{
    // ARRIVA ALL'INGRESSO DELLA DISCO
    printf("Il cliente %ld arriva fuori dalla discoteca\n", pthread_self());
    cliente_coda_fuori(&discoteca);

    printf("Il cliente %ld arriva dentro la discoteca\n", pthread_self());

    // VADO A FARE IL BIGLIETTO DALLA CASSIERA
    cliente_coda_dentro(&discoteca);
    printf("Il cliente %ld è in cassa e sta pagando il biglietto\n", pthread_self());

    // PAGO E RITIRO IL BIGLIETTO
    cliente_esco_coda(&discoteca);
    // A BALLARE
    return NULL;
}

void *cassiera(void *arg)
{
    while (1)
    {
        cassiera_attesa_cliente(&discoteca);
        // EMISSIONE BIGLIETTO E RISCOSSIONE SINGOLO CLIENTE
        cassiera_cliente_servito(&discoteca);
        // METTO A POSTO I SOLDINI
    }
    return NULL;
}
int main()
{
    pthread_attr_t a;
    pthread_t p;

    /* inizializzo il mio sistema */
    init_discoteca(&discoteca);

    /* inizializzo i numeri casuali, usati nella funzione pausetta */
    srand(555);

    pthread_attr_init(&a);

    /* non ho voglia di scrivere 10000 volte join! */
    pthread_attr_setdetachstate(&a, PTHREAD_CREATE_DETACHED);

    pthread_create(&p, &a, cassiera, (void *)"1");

    int i = 0;
    while (i < NTHREADS)
    {
        pthread_create(&p, &a, cliente, (void *)"1");
        i++;
    }

    pthread_attr_destroy(&a);
    /* aspetto 10 secondi prima di terminare tutti quanti */
    sleep(10);
    printf("%s", "Chiusura!\n");

    return 0;
}