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
    sem_t mutex;
    sem_t fuori;    // per i thread bloccati fuori
    sem_t cassa;    // per i thread che sono dentro ma che attendono alla cassa
    sem_t cassiera; // la cassiera si blocca in attesa dei clienti

    int bfuori, bcassa;
    int ndentro; // assumiamo che in attesa alla cassa possano stare fino alla metà dei thread totali. --> discriminiamo chi aspetta fuori e chi dentro

} discoteca;

void init_discoteca(struct discoteca_t *d)
{

    sem_init(&discoteca.mutex, 0, 1);
    sem_init(&discoteca.fuori, 0, 0);
    sem_init(&discoteca.cassa, 0, 0);
    sem_init(&discoteca.cassiera, 0, 0);

    discoteca.bfuori = discoteca.ndentro = discoteca.bcassa = 0;
}

void cliente_coda_fuori(struct discoteca_t *d)
{
    // arrivo alla discoteca, è necessario vedere se qualcuno è già fuori ad attendere altrimenti entro.
    sem_wait(&d->mutex);
    printf("Il cliente %ld > arrivato fuori dalla discoteca ci sono %d in attesa fuori e dentro ce ne sono %d\n", pthread_self(), d->bfuori, d->ndentro);
    if (d->ndentro == (NTHREADS / 2) - 1)
    {
        // mi blocco
        printf("Il cliente %ld > Devo aspettare fuori\n", pthread_self());
        d->bfuori++;
    }
    else
    {
        // non mi blocco fuori
        printf("Il cliente %ld > Posso entrare dentro (non c'è nessuno)\n", pthread_self());

        sem_post(&d->fuori);
        d->ndentro++;
    }
    sem_post(&d->mutex);
    sem_wait(&d->fuori); // qui mi sveglierà l'ultimo che è in coda dentro e che ha pagato.
}

void cliente_coda_dentro(struct discoteca_t *d)
{
    // arrivo dentro discoteca, è necessario vedere se qualcuno è in coda per la cassa d attendere altrimenti entro.
    sem_wait(&d->mutex);
    if (d->bcassa != 0)
    {
        // mi blocco
        printf("Il cliente %ld > In cassa c'è qualcuno, attendo...\n", pthread_self());

        d->bcassa++;
    }
    else
    {
        // non mi blocco in coda e sveglio la cassiera
        printf("Il cliente %ld > In cassa non c'è nessuno, vado dalla cassiera\n", pthread_self());

        sem_post(&d->cassa);
    }
    sem_post(&d->mutex);
    sem_wait(&d->cassa); // qui mi sveglierà chi ha pagato
    sem_post(&d->cassiera);
}

void cliente_esco_coda(struct discoteca_t *d)
{
    sem_wait(&d->mutex);
    printf("Il cliente %ld > Esco dalla disco\n", pthread_self());

    // devo occuparmi di svegliare qualcuno. QUELLI FUORI NON È IMPORTANTE L'ORDINE POSSO SVEGLIARLI ANCHE TUTTI
    d->ndentro--;

    int i = 0;
    if (d->ndentro != 0)
    {
        printf("Il cliente %ld > Sveglio uno in cassa\n", pthread_self());

        d->bcassa--;
        sem_post(&d->cassa);
    }
    else if (d->ndentro == 0 && d->bfuori > 0)
    {
        // sono l'ultimo che era dentro
        printf("Il cliente %ld > Chiamo quelli fuori che sono %d e dentro ne stanno altri %d\n", pthread_self(), d->bfuori, (NTHREADS / 2) - 1);

        while (d->ndentro < (NTHREADS / 2) - 1 && d->bfuori > 0)
        {
            sem_post(&d->fuori);
            ++i;
            d->bfuori--;
            d->ndentro++;
        }
    }
    printf("Il cliente %ld > Me ne vado a ballare\n", pthread_self());

    sem_post(&d->mutex);
}

void cassiera_attesa_cliente(struct discoteca_t *d)
{
    sem_post(&d->cassa);
    printf("Cassiera > Sono in attesa di un cliente.\n");
    sem_wait(&d->cassiera);
}

void cassiera_cliente_servito(struct discoteca_t *d)
{
    printf("Cassiera > Biglietto emesso e pagato, cliente servito.\n");
}

void *cliente(void *arg)
{
    // ARRIVA ALL'INGRESSO DELLA DISCO
    printf("Il cliente %ld arriva fuori dalla discoteca\n", pthread_self());
    pausetta();
    cliente_coda_fuori(&discoteca);
    pausetta();

    printf("Il cliente %ld arriva dentro la discoteca\n", pthread_self());

    // VADO A FARE IL BIGLIETTO DALLA CASSIERA
    pausetta();
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
        pausetta();
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