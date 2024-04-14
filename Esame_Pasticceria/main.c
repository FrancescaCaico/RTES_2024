#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdbool.h>

#define N 3
int nanosleep(const struct timespec *req, struct timespec *rem);

void pausetta(void)
{
    struct timespec t;
    t.tv_sec = 0;
    t.tv_nsec = (rand() % 10 + 1) * 1000000;
    nanosleep(&t, NULL);
}

struct pasticceria_t
{
    sem_t mutex;
    sem_t pasticcere;
    sem_t commesso;
    sem_t clienti;

    int b_clienti, torte_pronte, b_comm_cassa, b_comm_torte, b_past;

} pasticceria;

void init_pasticceria(struct pasticceria_t *p)
{
    sem_init(&p->mutex, 0, 1);
    sem_init(&p->pasticcere, 0, 0);
    sem_init(&p->commesso, 0, 0);
    sem_init(&p->clienti, 0, 0);

    p->b_clienti = p->b_comm_cassa = p->b_comm_torte = p->torte_pronte = p->b_past = 0;
}

void cuoco_inizio_torta(struct pasticceria_t *p)
{
    sem_wait(&p->mutex);
    if (p->torte_pronte < N)
    {
        // posso ancora fare un'altra torta
        printf("CUOCO --> LE TORTE SONO %d POSSO FARNE UN'ALTRA (limite --> %d)\n", p->torte_pronte, N);

        sem_post(&p->pasticcere);
    }
    else
    {
        printf("CUOCO --> LE TORTE SONO %d SIAMO AL LIMITE (%d)\n", p->torte_pronte, N);

        p->b_past = 1; // il pasticcere si blocca
    }
    sem_post(&p->mutex);
    sem_wait(&p->pasticcere);
}

void cuoco_fine_torta(struct pasticceria_t *p)
{
    sem_wait(&p->mutex);
    p->torte_pronte++;
    printf("CUOCO --> TORTA n%d finita\n", p->torte_pronte);
    if (p->b_comm_torte)
    {
        printf("CUOCO --> SVEGLIO IL COMMESSO ORA CHE LE TORTE SONO %d\n", p->torte_pronte);
        p->b_comm_torte = 0;
        sem_post(&p->commesso);
    }

    sem_post(&p->mutex);
}

void commesso_prendo_torta(struct pasticceria_t *p)
{
    sem_wait(&p->mutex);
    if (p->torte_pronte == 0 && p->b_past)
    {
        // sveglio il pasticcere per fargli fare una torta e mi metto in attesa
        printf("COMMESSO --> SVEGLIO IL PASTICCERE ORA CHE LE TORTE SONO %d\n", p->torte_pronte);

        p->b_past = 0;
        p->b_comm_torte = 1;
        sem_post(&p->pasticcere);
    }
    else
    {
        printf("COMMESSO --> CI SONO ANCORA %d TORTE\n", p->torte_pronte);

        sem_post(&p->commesso);
    }
    sem_post(&p->mutex);
    sem_wait(&p->commesso);
}

void commesso_vendo_torta(struct pasticceria_t *p)
{
    sem_wait(&p->mutex);
    p->torte_pronte--;
    if (p->b_clienti)
    {
        p->b_clienti--;
        sem_post(&p->commesso);
    }
    else
    {
        printf("COMMESSO --> NON C'È NESSUNO, ASPETTO UN CLIENTE\n");
        p->b_comm_cassa = 1;
    }
    sem_post(&p->mutex);
    sem_wait(&p->commesso);
    sem_post(&p->clienti);
    printf("COMMESSO --> SERVO UN CLIENTE\n");
}

void cliente_acquisto(struct pasticceria_t *p)
{
    sem_wait(&p->mutex);
    if (p->b_comm_cassa)
    {
        // sono il primo cliente
        printf("CLIENTE 1 (%ld) --> CHIAMO IL COMMESSO\n", pthread_self());
        p->b_comm_cassa = 0;
        sem_post(&p->commesso);
        // sem_post(&p->clienti); // io non mi blocco perchè sono il primo.
    }
    else
    {
        printf("CLIENTE %ld --> MI METTO IN ATTESA\n", pthread_self());
        p->b_clienti++;
    }
    sem_post(&p->mutex);
    sem_wait(&p->clienti);
}

void *cuoco(void *arg)
{
    while (1)
    {
        cuoco_inizio_torta(&pasticceria);
        cuoco_fine_torta(&pasticceria);
    }
}

void *commesso(void *arg)
{

    while (1)
    {
        commesso_prendo_torta(&pasticceria);
        commesso_vendo_torta(&pasticceria);
    }

    return 0;
}

void *un_cliente(void *arg)
{

    while (1)
    {
        printf("Cliente %ld > Mi reco in pasticceria...\n", pthread_self());
        cliente_acquisto(&pasticceria);
        printf("Cliente %ld > Vado a casa\n", pthread_self());
        pausetta();
    }
    return 0;
}

int main()
{
    pthread_attr_t a;
    pthread_t p;

    /* inizializzo il mio sistema */
    init_pasticceria(&pasticceria);

    /* inizializzo i numeri casuali, usati nella funzione pausetta */
    srand(555);

    pthread_attr_init(&a);

    /* non ho voglia di scrivere 10000 volte join! */
    pthread_attr_setdetachstate(&a, PTHREAD_CREATE_DETACHED);

    pthread_create(&p, &a, cuoco, (void *)"1");
    pthread_create(&p, &a, commesso, (void *)"1");

    int i = 0;
    while (i < 8)
    {
        pthread_create(&p, &a, un_cliente, (void *)"1");
        i++;
    }

    pthread_attr_destroy(&a);
    /* aspetto 10 secondi prima di terminare tutti quanti */
    sleep(10);
    printf("%s", "Chiusura!\n");

    return 0;
}