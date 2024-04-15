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
    pthread_mutex_t mutex;
    pthread_cond_t pasticcere;
    pthread_cond_t commesso;
    pthread_cond_t clienti;

    int b_clienti, torte_pronte, b_comm_cassa, b_comm_torte, b_past;

} pasticceria;

void init_pasticceria(struct pasticceria_t *p)
{
    pthread_mutexattr_t mattr;
    pthread_condattr_t condattr;

    pthread_mutexattr_init(&mattr);
    pthread_condattr_init(&condattr);

    pthread_mutex_init(&p->mutex, &mattr);
    pthread_cond_init(&p->pasticcere, &condattr);
    pthread_cond_init(&p->commesso, &condattr);
    pthread_cond_init(&p->clienti, &condattr);

    pthread_mutexattr_destroy(&mattr);
    pthread_condattr_destroy(&condattr);

    p->b_clienti = p->b_comm_cassa = p->b_comm_torte = p->torte_pronte = p->b_past = 0;
}

void cuoco_inizio_torta(struct pasticceria_t *p)
{
    pthread_mutex_lock(&p->mutex);
    while (p->torte_pronte == N)
    {
        // non posso ancora fare un'altra torta
        printf("CUOCO --> LE TORTE SONO %d SIAMO AL LIMITE (%d)\n", p->torte_pronte, N);
        p->b_past = 1; // il pasticcere si blocca
        pthread_cond_wait(&p->pasticcere, &p->mutex);
        p->b_past = 0; // il pasticcere si sblocca
    }
    pthread_mutex_unlock(&p->mutex);
}

void cuoco_fine_torta(struct pasticceria_t *p)
{
    pthread_mutex_lock(&p->mutex);
    p->torte_pronte++;
    printf("CUOCO --> TORTA n%d finita\n", p->torte_pronte);
    if (p->b_comm_torte)
    {
        printf("CUOCO --> SVEGLIO IL COMMESSO ORA CHE LE TORTE SONO %d\n", p->torte_pronte);

        pthread_cond_signal(&p->commesso);
    }

    pthread_mutex_unlock(&p->mutex);
}

void commesso_prendo_torta(struct pasticceria_t *p)
{
    pthread_mutex_lock(&p->mutex);
    while (p->torte_pronte == 0 && p->b_past)
    {
        // sveglio il pasticcere per fargli fare una torta e mi metto in attesa
        printf("COMMESSO --> SVEGLIO IL PASTICCERE ORA CHE LE TORTE SONO %d\n", p->torte_pronte);

        p->b_comm_torte = 1;
        pthread_cond_signal(&p->pasticcere);
        pthread_cond_wait(&p->commesso, &p->mutex);
        p->b_comm_torte = 0;
    }

    printf("COMMESSO --> CI SONO ANCORA %d TORTE\n", p->torte_pronte);
    pthread_mutex_unlock(&p->mutex);
}

void commesso_vendo_torta(struct pasticceria_t *p)
{
    pthread_mutex_lock(&p->mutex);

    while (!p->b_clienti)
    {
        p->b_comm_cassa = 1;
        printf("COMMESSO --> CI SONO %d CLIENTI IN ATTESA, **MI BLOCCO**\n", p->b_clienti);
        pthread_cond_wait(&p->commesso, &p->mutex);
        p->b_comm_cassa = 0;
    }
    printf("COMMESSO --> CI SONO %d CLIENTI IN ATTESA\n", p->b_clienti);

    printf("COMMESSO --> SERVO UN CLIENTE\n");
    pthread_cond_signal(&p->clienti);
    p->torte_pronte--;
    pthread_mutex_unlock(&p->mutex);
}

void cliente_acquisto(struct pasticceria_t *p)
{
    pthread_mutex_lock(&p->mutex);

    if (p->b_comm_cassa)
    {
        printf("CLIENTE 1 (%ld) --> CHIAMO IL COMMESSO\n", pthread_self());
        p->b_clienti++;
        pthread_mutex_unlock(&p->mutex);
        pthread_cond_signal(&p->commesso);
        pthread_mutex_lock(&p->mutex);
        p->b_clienti--;
    }
    else
    {
        while (p->b_clienti)
        {
            p->b_clienti++;
            printf("CLIENTE %ld --> MI METTO IN ATTESA (%d)\n", pthread_self(), p->b_clienti);

            pthread_cond_wait(&p->clienti, &p->mutex);
            p->b_clienti--;
        }
    }

    pthread_mutex_unlock(&p->mutex);
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