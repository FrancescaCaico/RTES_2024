#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>
#include <pthread.h>

#define BALLERINI 10 // il numero di thread
#define CAPACITA 4   // la capacità interna della biglietteria

struct discoteca_t
{
    pthread_mutex_t mutex;   // per l'accesso m.esclusivo delle risorse manipolate
    pthread_cond_t entrata;  // per l'eventuale attesa dovuta al fatto che dentro ci sono persone = capacità
    pthread_cond_t cassa;    // per l'eventuale attesa dovuta al fatto che c'è la fila alla cassa
    pthread_cond_t cassiera; // per l'attesa della cassiera di eventuali clienti

    int ndentro, bfuori, bcassa, cassa_disponibile;
    int cassiera_b;
    int cliente_presente;

} discoteca;

void init_discoteca(struct discoteca_t *d)
{
    pthread_mutexattr_t m;
    pthread_condattr_t c;

    pthread_mutexattr_init(&m);
    pthread_condattr_init(&c);

    pthread_mutex_init(&d->mutex, &m);
    pthread_cond_init(&d->entrata, &c);
    pthread_cond_init(&d->cassa, &c);
    pthread_cond_init(&d->cassiera, &c);

    d->ndentro = d->bfuori = d->bcassa = d->cassa_disponibile = 0;
    d->cliente_presente = 0;
    d->cassiera_b = 0;

    pthread_mutexattr_destroy(&m);
    pthread_condattr_destroy(&c);
}

void cliente_coda_fuori(struct discoteca_t *d)
{
    pthread_mutex_lock(&d->mutex);
    // devo controllare se posso entrare all'interno della discoteca o se è stata raggiunta la massima capacità-
    while (d->ndentro == CAPACITA)
    {
        // NON POSSO ENTRARE MI BLOCCO SULLA CODA FUORI
        d->bfuori++;
        printf("Cliente %ld > Aspetto fuori\n", pthread_self());
        pthread_cond_wait(&d->entrata, &d->mutex);
        d->bfuori--;
    }
    // arrivati a questo punto posso entrare dentro
    printf("Cliente %ld > Sono entrato\n", pthread_self());
    d->ndentro++;
    pthread_mutex_unlock(&d->mutex);
}

void cliente_coda_dentro(struct discoteca_t *d)
{
    pthread_mutex_lock(&d->mutex);
    // devo controllare se c'è già gente in coda --> mi blocco, se in coda non c'è nessuno devo occuparmi di svegliare la cassiera in attesa dei clienti
    printf("Cliente %ld > appena rrivato la cassa è %d\n", pthread_self(), d->cassa_disponibile);
    while (!d->cassa_disponibile)
    {

        d->bcassa++;
        pthread_cond_wait(&d->cassa, &d->mutex); // qui mi sveglia la cassiera che controlla se c'è qualcuno bloccato in cassa prima di sospendersi.
        d->bcassa--;
    }
    d->cliente_presente = 1;
    if (d->cassiera_b)
    {
        printf("Cliente %ld > Sveglio la cassiera \n", pthread_self());
        pthread_cond_signal(&d->cassiera);
    }
    d->cassa_disponibile = 0;
    pthread_mutex_unlock(&d->mutex);
}

void cliente_esco_coda(struct discoteca_t *d)
{
    pthread_mutex_lock(&d->mutex);
    d->ndentro--;
    // devo uscire fuori, mi devo occupare di far entrare gli altri solo se sono l'ultimo in cassa.
    if (!d->ndentro)
    {
        pthread_cond_broadcast(&d->entrata);
    }

    pthread_mutex_unlock(&d->mutex);
}

void cassiera_attesa_cliente(struct discoteca_t *d)
{
    // qundo si blocca la cassiera? se nessun cliente è presente
    pthread_mutex_lock(&d->mutex);
    d->cassa_disponibile = 1;
    pthread_cond_signal(&d->cassa);
    while (!d->cliente_presente)
    {
        d->cassiera_b++;
        pthread_cond_wait(&d->cassiera, &d->mutex);
        d->cassiera_b--;
    }
    d->cliente_presente = 0;
    pthread_mutex_unlock(&d->mutex);
}

void cassiera_cliente_servito(struct discoteca_t *d)
{
    pthread_mutex_lock(&d->mutex);

    printf("Cassiera > Cliente servito\n");
    if (d->bcassa)
    {
        pthread_cond_signal(&d->cassa);
    }

    pthread_mutex_unlock(&d->mutex);
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
    printf("Il cliente %ld è in pista!!!\n", pthread_self());

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
    while (i < BALLERINI)
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
