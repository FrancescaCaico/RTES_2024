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
    sem_t pasticcere; // in cui il pasticcere si bloccherà quando non ci sarà più spazio sugli scaffali
    sem_t commesso;   // su cui il commesso dovrà bloccarsi in attesa di un cliente
    sem_t clienti;    // su cui il cliente dovrà bloccarsi in attesa di essere servito.

    int clienti_attesa, torte_pronte;
    bool bloccato_cassa;
    bool bloccato_torte;

} pasticceria;

void init_pasticceria(struct pasticceria_t *p)
{

    sem_init(&p->mutex, 0, 1);
    sem_init(&p->pasticcere, 0, 0);
    sem_init(&p->commesso, 0, 0);
    sem_init(&p->clienti, 0, 0);

    p->clienti_attesa = p->torte_pronte = 0;
    p->bloccato_cassa = p->bloccato_torte = 0;
}

void cuoco_inizio_torta(struct pasticceria_t *p)
{
    sem_wait(&p->mutex);

    printf("Torte già fatte %d\n", p->torte_pronte);

    if (p->torte_pronte < N)
    {
        printf("Pasticcere >> Non siamo al limite\n");
        sem_post(&p->pasticcere);
    }
    else
    {
        printf("Abbiamo superato il limite\n");
    }
    sem_post(&p->mutex);
    sem_wait(&p->pasticcere);
}

void cuoco_fine_torta(struct pasticceria_t *p)
{
    sem_wait(&p->mutex);
    p->torte_pronte++;
    printf("Cuoco >> Torte pronte attuali %d\n", p->torte_pronte);
    printf("Ho finito una torta il commesso è %d\n", p->bloccato_torte);
    if (p->bloccato_torte)
    {
        // se il commesso è in attesa che ci sia una torta per incartarla lo sveglio
        printf("*Cuoco*> sveglio un commesso in attesa della torta da servire\n");
        p->bloccato_torte = 0;
        sem_post(&p->commesso);
    }

    sem_post(&p->mutex);
}

void commesso_prendo_torta(struct pasticceria_t *p)
{
    sem_wait(&p->mutex);
    printf("Le torte attualmente pronte sono %d\n", p->torte_pronte);
    p->torte_pronte--;
    if (p->torte_pronte == 0)
    {
        p->bloccato_torte = 1;
        printf("Sveglio il psticcere (t.p. %d)\n", p->torte_pronte);
        sem_post(&p->pasticcere);
    }
    else
    {
        sem_post(&p->commesso);
    }
    sem_post(&p->mutex);
    sem_wait(&p->commesso);
}

void commesso_vendo_torta(struct pasticceria_t *p)
{
    sem_wait(&p->mutex);
    printf("Posso vendere una torta\n");
    if (p->clienti_attesa)
    {
        printf("Ci sono clienti in attesa\n");

        p->clienti_attesa--;
        sem_post(&p->commesso);
        sem_post(&p->clienti);
    }
    else
    {
        printf("Non ci sono clienti in attesa, mi bloccherò aspettando il primo\n");
        p->bloccato_cassa = 1;
    }
    sem_post(&p->mutex);
    sem_wait(&p->commesso);
}

void cliente_acquisto(struct pasticceria_t *p)
{
    sem_wait(&p->mutex);
    printf("Ho preso il mutex\n");
    printf("Ci sono %d persone avanti a me\n", p->clienti_attesa);
    if (p->bloccato_cassa)
    {
        // sveglio il cassiere
        printf("Sono il primo cliente sveglio il cassiere\n");
        p->bloccato_cassa = 0;
        sem_post(&p->commesso);
    }
    else
    {

        p->clienti_attesa++;
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
        pausetta();
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