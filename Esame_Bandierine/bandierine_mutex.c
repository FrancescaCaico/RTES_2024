
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

struct bandierine_t
{
    pthread_mutex_t mutex;
    pthread_cond_t giocatore[2];
    pthread_cond_t giudice;

    int via; // se il giudice ha dato il via
    int presa;
    int al_via;
    int vincitore;

} bandierine;

void init_bandierine(struct bandierine_t *b)
{
    pthread_mutexattr_t m;
    pthread_condattr_t c;

    pthread_mutexattr_init(&m);
    pthread_condattr_init(&c);

    pthread_mutex_init(&b->mutex, &m);
    pthread_cond_init(&b->giocatore[0], &c);
    pthread_cond_init(&b->giocatore[1], &c);

    b->al_via = 0;
    b->presa = -1;
    b->vincitore = -1;

    pthread_mutexattr_destroy(&m);
    pthread_condattr_destroy(&c);
}

void via(struct bandierine_t *b)
{
    pthread_mutex_lock(&b->mutex);
    b->via = 1;
    pthread_cond_signal(&b->giocatore[0]);
    pthread_cond_signal(&b->giocatore[1]);
    pthread_mutex_unlock(&b->mutex);
    printf("Giudice >> VIAAA\n");
}

void attendi_giocatori(struct bandierine_t *b)
{
    pthread_mutex_lock(&b->mutex);
    while (b->al_via == 2)
    {
        pthread_cond_wait(&b->giudice, &b->mutex);
    }

    pthread_mutex_unlock(&b->mutex);
}

void attendi_il_via(struct bandierine_t *b, int n)
{
    pthread_mutex_lock(&b->mutex);
    b->al_via++;
    pthread_cond_signal(&b->giudice);
    while (!b->via)
    {
        // non ha dato il via, attendo
        pthread_cond_wait(&b->giocatore[n], &b->mutex);
    }
    pthread_mutex_unlock(&b->mutex);
}

int bandierina_presa(struct bandierine_t *b, int n)
{
    int ritorno = 0;
    pthread_mutex_lock(&b->mutex);

    if (b->presa == -1)
    {
        // mi prendo la bandierina!!
        b->presa = n;
        ritorno = 1;
    }

    pthread_mutex_unlock(&b->mutex);
    return ritorno;
}

int sono_salvo(struct bandierine_t *b, int n)
{
    int ritorno = 0;
    // sono salvo se ho preso la bandierina e l'altro giocatore non è arrivato al traguardo.
    pthread_mutex_lock(&b->mutex);
    if (b->vincitore == -1)
    {
        // non è arrivato nessuno al traguardo, sono io il vincitore
        b->vincitore = n;
        // sveglio il giudice in attesa
        pthread_cond_signal(&b->giudice);
        ritorno = 1;
    }
    pthread_mutex_unlock(&b->mutex);
    return ritorno;
}

int ti_ho_preso(struct bandierine_t *b, int n)
{
    return sono_salvo(b, n);
}

int risultato_gioco(struct bandierine_t *b)
{
    int ritorno;
    pthread_mutex_lock(&b->mutex);
    while (b->vincitore == -1)
    {
        pthread_cond_wait(&b->giudice, &b->mutex);
    }
    ritorno = b->vincitore;
    pthread_mutex_unlock(&b->mutex);
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