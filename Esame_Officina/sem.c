#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/wait.h>

#define N 8
#define NCLIENTI 30
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
    sem_t mutex;
    sem_t operaio[N];
    sem_t attesa_riparazione;
    sem_t attesa_ufficio;

    int b_ufficio, dentro_ufficio;
} o;

void init_officina(struct officina_t *officina)
{
    sem_init(&officina->mutex, 0, 1);
    sem_init(&officina->attesa_riparazione, 0, 0);
    sem_init(&officina->attesa_ufficio, 0, 0);

    for (int i = 0; i < N; i++)
    {
        sem_init(&officina->operaio[i], 0, 0);
    }
    officina->b_ufficio = officina->dentro_ufficio = 0;
}

void cliente_arrivo(struct officina_t *officina, int r)
{
    sem_wait(&officina->mutex);

    if (officina->dentro_ufficio == 0)
    {
        // posso entrare e richiedere il servizio.
        sem_post(&officina->attesa_ufficio);
        officina->dentro_ufficio++;
    }
    else
    {
        printf("%ld > In attesa che l'ufficio si liberi\n", pthread_self());
        officina->b_ufficio++;
    }

    sem_post(&officina->mutex);
    sem_wait(&officina->attesa_ufficio);

    // sveglio l'operaio che deve eseguire la riparazione.
    sem_post(&officina->operaio[r]);
}

void cliente_attesafineservizio(struct officina_t *officina)
{
    printf("%ld > in attesa della fine del servizio\n", pthread_self());
    sem_wait(&officina->mutex);
    printf("%ld > Servizio finito... Vado a casa\n", pthread_self());
    officina->dentro_ufficio--;
    if (officina->b_ufficio)
    {
        officina->b_ufficio--;
        officina->dentro_ufficio++;
        sem_post(&officina->attesa_ufficio);
    }
    sem_post(&officina->mutex);
}

void operaio_attesacliente(struct officina_t *officina, int r)
{

    // mi metto in attesa di un cliente che voglia il mio tipo di riparazione
    sem_wait(&officina->operaio[r]);
    printf("L'operaio %ld > Qualcuno ha chiesto la riparazione %d che svolgo solo io\n", pthread_self(), r);
}

void operaio_fineservizio(struct officina_t *officina)
{

    // mi metto in attesa di un cliente che voglia il mio tipo di riparazione
    printf("L'operaio %ld > Ho terminato il servizio richiesto, avviso il cliente\n", pthread_self());
    sem_post(&officina->attesa_riparazione);
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

        operaio_fineservizio(&o);
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