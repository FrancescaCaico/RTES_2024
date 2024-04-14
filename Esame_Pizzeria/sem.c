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
#include <stdint.h>
#include <limits.h>

#define NCLIENTI 8
int nanosleep(const struct timespec *req, struct timespec *rem);

void pausetta(void)
{
    struct timespec t;
    t.tv_sec = 0;
    t.tv_nsec = (rand() % 10 + 1) * 10000000;
    nanosleep(&t, NULL);
}

struct codaclienti_t
{
    sem_t mutex;
    sem_t pizzaiolo;
    sem_t clienti[NCLIENTI];

    int in_attesa;
    int ordine_cliente[NCLIENTI];
    bool pizzaiolo_bloccato;
    int cliente_scelto;
} coda;

void init_codaclienti(struct codaclienti_t *codaclienti)
{
    sem_init(&codaclienti->mutex, 0, 1);
    sem_init(&codaclienti->pizzaiolo, 0, 0);
    for (int i = 0; i < NCLIENTI; i++)
    {
        /* code */
        sem_init(&codaclienti->clienti[i], 0, 0);
        codaclienti->ordine_cliente[i] = -1; // inizialmente nessuno ha ordinato
    }
    codaclienti->in_attesa = 0;
    codaclienti->pizzaiolo_bloccato = 0;
    codaclienti->cliente_scelto = -1; // per tenere traccia del cliente che sto servendo
}

int cerca_minimo_ordine(struct codaclienti_t *coda)
{
    int min = -1;
    int npizze = INT_MAX;
    for (int i = 0; i < NCLIENTI; i++)
    {
        printf("Cliente %d : %d\n", i, coda->ordine_cliente[i]);
    }

    for (int i = 0; i < NCLIENTI; ++i)
    {
        if (npizze > coda->ordine_cliente[i] && coda->ordine_cliente[i] != -1)
        {
            npizze = coda->ordine_cliente[i];
            min = i;
        }
    }
    return min;
}

void ordina_pizze(struct codaclienti_t *coda, int numero_pizze, int cliente)
{
    sem_wait(&coda->mutex);
    coda->ordine_cliente[cliente] = numero_pizze;
    coda->in_attesa++;
    if (coda->pizzaiolo_bloccato)
    {
        printf("Cliente %d > Il pizzaiolo dormiva, lo sveglio\n", cliente);

        coda->pizzaiolo_bloccato = 0;
        sem_post(&coda->pizzaiolo);
    }
    sem_post(&coda->mutex);
}

void ritira_pizze(struct codaclienti_t *coda, int cliente)
{
    printf("Cliente %d, mi blocco in attesa del mio ordine\n", cliente);
    sem_wait(&coda->clienti[cliente]);
    printf("Cliente %d > Ritiro il mio ordine\n", cliente);
}

void prossima_pizza(struct codaclienti_t *coda)
{
    sem_wait(&coda->mutex);
    if (!coda->in_attesa)
    {
        printf("Non c'è ancora nessun cliente\n");

        coda->pizzaiolo_bloccato = 1;
        sem_post(&coda->mutex);
        sem_wait(&coda->pizzaiolo);
        printf("E' arrivato un cliente\n");
    }

    if (coda->cliente_scelto == -1)
    {
        // non ho ancora scelto un cliente da servire...
        coda->cliente_scelto = cerca_minimo_ordine(coda);
        printf("Ho scelto di servire il cliente n.%d con %d pizze\n", coda->cliente_scelto, coda->ordine_cliente[coda->cliente_scelto]);
    }
    sem_post(&coda->mutex);
}

void consegna_pizza(struct codaclienti_t *coda)
{
    sem_wait(&coda->mutex);
    printf("Pizza pronta per il cliente %d, ne restano %d da preparare\n", coda->cliente_scelto, coda->ordine_cliente[coda->cliente_scelto]);
    coda->ordine_cliente[coda->cliente_scelto]--;

    if (!coda->ordine_cliente[coda->cliente_scelto])
    {
        printf("Ho finito l'ordine del cliente n.%d\n", coda->cliente_scelto);

        coda->in_attesa--;
        coda->ordine_cliente[coda->cliente_scelto] = -1;
        sem_post(&coda->clienti[coda->cliente_scelto]);
        coda->cliente_scelto = -1;
    }
    sem_post(&coda->mutex);
}

void *cliente(void *arg)
{

    int cliente = (intptr_t)arg;
    while (1)
    {
        int n;
        if ((n = (rand() % 10)))
        {
            printf("Cliente %d > Ordino %d pizze\n", cliente, n);
            ordina_pizze(&coda, n, cliente);
            printf("Cliente %d > Attendo il mio ordine\n", cliente);
            ritira_pizze(&coda, cliente);
            printf("Cliente %d > Ritirato!\n", cliente);
            pausetta();
        }
    }
}

void *pizzaiolo(void *arg)
{

    while (1)
    {
        prossima_pizza(&coda);

        consegna_pizza(&coda);
        pausetta();
    }
}
int main()
{
    pthread_attr_t a;
    pthread_t p;

    /* inizializzo il mio sistema */
    init_codaclienti(&coda);

    /* inizializzo i numeri casuali, usati nella funzione pausetta */
    srand(555);

    pthread_attr_init(&a);

    /* non ho voglia di scrivere 10000 volte join! */
    pthread_attr_setdetachstate(&a, PTHREAD_CREATE_DETACHED);

    pthread_create(&p, &a, pizzaiolo, NULL);

    intptr_t i = 0;
    while (i < NCLIENTI)
    {
        pthread_create(&p, &a, cliente, (void *)i);
        i++;
    }

    pthread_attr_destroy(&a);
    /* aspetto 10 secondi prima di terminare tutti quanti */
    sleep(15);
    printf("%s", "Chiusura!\n");

    return 0;
}