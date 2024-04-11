/*Esercizio 7
Un negozio di barbieri ha tre barbieri, e tre poltrone su cui siedono i clienti quando vengono per tagliarsi
la barba.
C'è una sala d'aspetto con un divano (max 4 persone; gli altri aspettano fuori dalla porta).
C'e' un cassiere, che può servire solamente un cliente (con barba tagliata) alla volta.
Scrivere il processo cliente che cerca di entrare nel negozio per farsi tagliare la barba,
Suggerimenti:
– considerare i barbieri, il cassiere ed il divano come risorse condivise.
– modellare il processo di taglio barba come un ciclo di SHAVING_ITERATIONS iterazioni
– modellare il processo di pagamento come un ciclo di PAYING_ITERATIONS iterazioni
– dopo che un cliente ha pagato esce (th thread muore) oppure, dopo un delay di alcuni secondi (usare la
primitiva sleep() ), si accoda nuovamente per farsi tagliare la barba.*/

#include <stdio.h>
#include <semaphore.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <time.h>
#include <stdbool.h>
#include <stdint.h>

#define NUM_BARBIERI 3
#define NUM_POLTRONE
#define CASSIERE 1
#define NUM_CLIENTI 10 // Numero di clienti che entrano nel negozio

#define SHAVING_ITERATIONS 8
#define PAYING_ITERATIONS 5

struct barbershop_t
{
    pthread_mutex_t mutex;

    pthread_cond_t attesa;
    pthread_cond_t divano;
    pthread_cond_t pagamento;

    int b_liberi, c_attivo, num_posti; // battivi corrisponde anche al numero di poltrone occupate e che quindi non possono essere usate.

} barbershop;

void initBarberShop(struct barbershop_t *barbershop)
{
    pthread_mutexattr_t mutex_attr;
    pthread_condattr_t cond_attr;

    pthread_mutexattr_init(&mutex_attr);
    pthread_condattr_init(&cond_attr);

    pthread_mutex_init(&barbershop->mutex, &mutex_attr);
    pthread_cond_init(&barbershop->divano, &cond_attr);    // per i 4 che attendono dentro di avere posto sulle poltrone
    pthread_cond_init(&barbershop->pagamento, &cond_attr); // per i clienti che già sono stati serviti e devono pagare ma il cassiere è impegnato
    pthread_cond_init(&barbershop->attesa, &cond_attr);    // per quelli in attesa fuori risvegliati quando un posto si libera sul divano.

    pthread_mutexattr_destroy(&mutex_attr);
    pthread_condattr_destroy(&cond_attr);

    barbershop->b_liberi = NUM_BARBIERI;
    barbershop->c_attivo = CASSIERE;
    barbershop->num_posti = 4;
}

void eseguiTaglio()
{
    for (int i = 0; i < SHAVING_ITERATIONS; ++i)
    {
    }
}

void pagaServizio()
{
    for (int i = 0; i < SHAVING_ITERATIONS; ++i)
    {
    }
}

void entraBarberShop()
{
    pthread_mutex_lock(&barbershop.mutex);

    // mi blocco se i divani sono pieni e quindi non posso entrare nè mettermi sul divano.
    while (barbershop.num_posti == 0)
    {
        // mi blocco fuori dalla porta sulla coda attesa.
        pthread_cond_wait(&barbershop.attesa, &barbershop.mutex);
    }
    // se sono qui, si è liberato un posto nel divano, posso dunque entrare --> decremento il numero di posti del divano.
    barbershop.num_posti--;
    // mi blocco se tutti i parrucchieri stanno operando su qualche thread.
    while (barbershop.b_liberi == 0)
    {
        // resto in attesa sul divano finchè un barbiere non ha finito e mi chiama.
        pthread_cond_wait(&barbershop.divano, &barbershop.mutex);
    }

    // mi hanno chiamato per il taglio.
    barbershop.b_liberi--;
    barbershop.num_posti++;
    // ho il compito di lasciare il divano in modo da far entrare qualcun'altro.
    pthread_cond_broadcast(&barbershop.attesa); // quelli che sono fuori

    // mi taglio i capelli.

    eseguiTaglio();

    // ho finito, rilascio il mio barbiere e mi dirigo a pagare dal cassiere.
    barbershop.b_liberi++;
    pthread_cond_signal(&barbershop.divano);
    //  finchè non c'è il cassiere devo attendere il mio turno sulla coda pagamento
    while (!barbershop.c_attivo)
    {
        pthread_cond_wait(&barbershop.pagamento, &barbershop.mutex);
    }
    // qui il cassiere ha concluso e pago.
    barbershop.c_attivo--;
    pagaServizio();

    // rilascio il cassiere
    barbershop.c_attivo++;
    // sveglio gli altri in coda
    pthread_cond_broadcast(&barbershop.pagamento); // quelli che sono fuori
    pthread_mutex_unlock(&barbershop.mutex);
}

void *cliente(void *arg)
{
    int cliente_id = (int)(intptr_t)arg;

    printf("Cliente %d: Cerco di entrare nel negozio.\n", cliente_id);
    entraBarberShop();
    printf("Cliente %d: Ho finito di tagliarmi la barba e pagare. Esco.\n", cliente_id);

    return NULL;
}

int main()
{
    pthread_t threads[NUM_CLIENTI];

    initBarberShop(&barbershop);

    for (int i = 0; i < NUM_CLIENTI; i++)
    {
        pthread_create(&threads[i], NULL, cliente, (void *)(intptr_t)i);
    }

    for (int i = 0; i < NUM_CLIENTI; i++)
    {
        pthread_join(threads[i], NULL);
    }

    return 0;
}