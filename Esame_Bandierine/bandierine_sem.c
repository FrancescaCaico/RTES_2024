
#include <stdio.h>
#include <semaphore.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <stdint.h>
#include <semaphore.h>
#include <time.h>

int nanosleep(const struct timespec *req, struct timespec *rem);

void pausetta(void)
{
    struct timespec t;
    t.tv_sec = 0;
    t.tv_nsec = (rand() % 10 + 1) * 1000000;
    nanosleep(&t, NULL);
}

struct bandierine_t
{
    sem_t mutex;
    sem_t giocatori[2];
    sem_t giudice;
    int blocked;
    int presa; // si riempie dell'indice di chi ha preso la bandierina
    int vincitore;
    int al_via, arrivati;

} bandierine;

void init_bandierine(struct bandierine_t *b)
{
    sem_init(&b->mutex, 0, 1);
    sem_init(&b->giudice, 0, 0);
    sem_init(&b->giocatori[0], 0, 0);
    sem_init(&b->giocatori[1], 0, 0);
    b->al_via = 0;
    b->presa = -1;
    b->vincitore = -1;
}

void via(struct bandierine_t *b)
{
    sem_post(&b->giocatori[0]);
    sem_post(&b->giocatori[1]);

    printf("Giudice >> VIAAA\n");
}

void attendi_giocatori(struct bandierine_t *b)
{
    sem_wait(&b->mutex);
    if (b->al_via == 2)
    {
        sem_post(&b->giudice);
    }

    sem_post(&b->mutex);
    sem_wait(&b->giudice);
}

void attendi_il_via(struct bandierine_t *b, int n)
{
    sem_wait(&b->mutex);
    b->al_via++;
    if (b->al_via == 2)
    {
        sem_post(&b->giudice);
    }
    sem_post(&b->mutex);
    sem_wait(&b->giocatori[n]);
}

int bandierina_presa(struct bandierine_t *b, int n)
{
    int ritorno = 0;
    sem_wait(&b->mutex);

    if (b->presa == -1)
    {
        // mi prendo la bandierina!!
        b->presa = n;
        ritorno = 1;
    }

    sem_post(&b->mutex);
    return ritorno;
}

int sono_salvo(struct bandierine_t *b, int n)
{
    int ritorno = 0;
    // sono salvo se ho preso la bandierina e l'altro giocatore non è arrivato al traguardo.
    sem_wait(&b->mutex);
    if (b->vincitore == -1)
    {
        // non è arrivato nessuno al traguardo, sono io il vincitore
        b->vincitore = n;
        // sveglio il giudice in attesa
        sem_post(&b->giudice);
        ritorno = 1;
    }
    sem_post(&b->mutex);
    return ritorno;
}

int ti_ho_preso(struct bandierine_t *b, int n)
{
    int ritorno = 0;
    // ho preso un giocatore se arrivo prima
    sem_wait(&b->mutex);
    if (b->vincitore == -1)
    {
        // non è arrivato nessuno al traguardo, sono io il vincitore
        b->vincitore = n;
        // sveglio il giudice in attesa
        sem_post(&b->giudice);
        ritorno = 1;
    }
    sem_post(&b->mutex);
    return ritorno;
}

int risultato_gioco(struct bandierine_t *b)
{
    int ritorno;
    sem_wait(&b->giudice); // mi blocco finchè il vincitore non mi sveglia
    sem_wait(&b->mutex);
    b->al_via = 0;
    ritorno = b->vincitore;
    sem_post(&b->mutex);
    return ritorno;
}

void *giocatore(void *arg)
{
    int numero_giocatore = (int)arg;

    printf("Giocatore %d >>Attendo il via...\n", numero_giocatore);
    attendi_il_via(&bandierine, numero_giocatore);
    if (bandierina_presa(&bandierine, numero_giocatore))
    {
        printf("Giocatore %d >> Ho preso la bandierina!\n", numero_giocatore);
        if (sono_salvo(&bandierine, numero_giocatore))
        {
            printf("Giocatore %d >> Sono salvo\n", numero_giocatore);
        }
    }
    else
    {
        if (ti_ho_preso(&bandierine, numero_giocatore))
        {
            printf("Giocatore %d >> Ti ho preso\n", numero_giocatore);
        }
    }
}

void *giudice(void *arg)
{

    attendi_giocatori(&bandierine);
    printf("Giudice >> PRONTI, ATTENTI...\n");
    via(&bandierine);
    printf("Il vincitore è: %d\n", risultato_gioco(&bandierine));
}

int main()
{
    pthread_t giocatori[2], giudice_thread;

    init_bandierine(&bandierine);

    pthread_create(&giudice_thread, NULL, giudice, NULL);
    pthread_create(&giocatori[0], NULL, giocatore, (void *)0);
    pthread_create(&giocatori[1], NULL, giocatore, (void *)1);

    pthread_join(giudice_thread, NULL);
    pthread_join(giocatori[0], NULL);
    pthread_join(giocatori[1], NULL);

    return 0;
}