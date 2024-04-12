#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/wait.h>

#define N 8
#define NCLIENTI 32
int nanosleep(const struct timespec *req, struct timespec *rem);

void pausetta(void)
{
    struct timespec t;
    t.tv_sec = 0;
    t.tv_nsec = (rand() % 10 + 1) * 1000000;
    nanosleep(&t, NULL);
}

struct officina_t
{
    pthread_mutex_t mutex;
    pthread_cond_t attesa_ufficio;
    pthread_cond_t attesa_riparazione;
    pthread_cond_t operaio[N];

    int b_ufficio, dentro_ufficio;

} o;

void init_officina(struct officina_t *officina)
{
    pthread_mutexattr_t mutex_attr;
    pthread_condattr_t cond_attr;

    pthread_mutexattr_init(&mutex_attr);
    pthread_condattr_init(&cond_attr);

    pthread_mutex_init(&officina->mutex, &mutex_attr);
    pthread_cond_init(&officina->attesa_riparazione, &cond_attr);
    pthread_cond_init(&officina->attesa_ufficio, &cond_attr);

    for (int i = 0; i < N; i++)
    {
        /* code */
        pthread_cond_init(&officina->operaio[i], &cond_attr);
    }

    pthread_mutexattr_destroy(&mutex_attr);
    pthread_condattr_destroy(&cond_attr);

    officina->b_ufficio = 0;
    officina->dentro_ufficio = -1; // forse dentro ufficio può anche non servire....
}

void cliente_arrivo(struct officina_t *officina, int r)
{
    pthread_mutex_lock(&officina->mutex);

    while (officina->dentro_ufficio != -1)
    {
        officina->b_ufficio++;
        printf("%ld > Aspetto fuori per il servizio %d\n", pthread_self(), r);
        pthread_cond_wait(&officina->attesa_ufficio, &officina->mutex);
        officina->b_ufficio--;
    }
    officina->dentro_ufficio = r;

    pthread_mutex_unlock(&officina->mutex);
    // ++officina->coda_riparazione[r];
    printf("%ld > Chiedo il servizio %d\n", pthread_self(), r);
    pthread_cond_signal(&officina->operaio[r]);
}

void cliente_attesafineservizio(struct officina_t *officina)
{
    pthread_mutex_lock(&officina->mutex);
    printf("%ld > in attesa della fine del servizio\n", pthread_self());
    pthread_cond_wait(&officina->attesa_riparazione, &officina->mutex);
    printf("%ld > Servizio finito... Vado a casa\n", pthread_self());
    officina->dentro_ufficio = -1;
    ;

    if (officina->b_ufficio)
    {
        pthread_cond_signal(&officina->attesa_ufficio);
    }
    pthread_mutex_unlock(&officina->mutex);
}

void operaio_attesacliente(struct officina_t *officina, int r)
{

    // mi metto in attesa di un cliente che voglia il mio tipo di riparazione
    pthread_mutex_lock(&officina->mutex);
    printf("Operaio che si occupa di r --> %d in attesa\n", r);
    while (officina->dentro_ufficio != r)
    {
        pthread_cond_wait(&officina->operaio[r], &officina->mutex);
    }
    printf("L'operaio %ld > Qualcuno ha chiesto la riparazione %d che svolgo solo io\n", pthread_self(), r);
    pthread_mutex_unlock(&officina->mutex);
}

void operaio_fineservizio(struct officina_t *officina)
{

    // mi metto in attesa di un cliente che voglia il mio tipo di riparazione
    pthread_mutex_lock(&officina->mutex);
    printf("L'operaio %ld > Ho terminato il servizio richiesto, avviso il cliente\n", pthread_self());

    pthread_cond_signal(&officina->attesa_riparazione);
    pthread_mutex_unlock(&officina->mutex);
}

void *cliente(void *arg)
{
    int r = rand() % N;
    cliente_arrivo(&o, r);
    cliente_attesafineservizio(&o);
    return NULL;
}

void *operaio(void *arg)
{
    // il tipo di operazione che può svolgere la passiamo come parametro dal main.
    int r = (intptr_t)arg;
    while (1)
    {

        operaio_attesacliente(&o, r);
        printf("Operaio che si occupa di r --> %d è arrivato un cliente\n", r);

        operaio_fineservizio(&o);
        printf("Operaio che si occupa di r --> %d ha finito il servizio\n", r);

        pausetta();
    }
}

int main()
{
    pthread_attr_t a;
    pthread_t p;

    /* inizializzo il mio sistema */
    init_officina(&o);

    /* inizializzo i numeri casuali, usati nella funzione pausetta */
    srand(555);

    pthread_attr_init(&a);

    /* non ho voglia di scrivere 10000 volte join! */
    pthread_attr_setdetachstate(&a, PTHREAD_CREATE_DETACHED);

    int i = 0;
    while (i < NCLIENTI)
    {
        pthread_create(&p, &a, cliente, NULL);
        i++;
    }
    intptr_t x = 0;
    while (x < N)
    {
        pthread_create(&p, &a, operaio, (void *)x);
        x++;
    }

    pthread_attr_destroy(&a);
    /* aspetto 10 secondi prima di terminare tutti quanti */
    sleep(10);
    printf("%s", "Chiusura!\n");

    return 0;
}