/*Esercizio 4
Scrivere un programma multi-thread che simuli il gioco della morra cinese. In tale programma ci devono
essere 3 thread:
- 2 thread simulano i giocatori;
- 1 thread simula l'arbitro.
Il thread arbitro ha il compito di:
1. "dare il via" ai due thread giocatori;
2. aspettare che ciascuno di essi faccia la propria mossa;
3. controllare chi dei due ha vinto, e stampare a video il risultato;
4. aspettare la pressione di un tasto da parte dell'utente;
5. ricominciare dal punto 1.
Ognuno dei due thread giocatori deve:
1. aspettare il "via" da parte del thread arbitro;
2. estrarre a caso la propria mossa;
3. stampare a video la propria mossa;
4. segnalare al thread arbitro di aver effettuato la mossa;
5. tornare al punto 1.
Per semplicita', assumere che la mossa sia codificata come un numero intero con le seguenti define:
#define CARTA 0
#define SASSO 1
#define FORBICE 2
e che esista un array di stringhe cosi' definito:
char *nomi_mosse[3] = {"carta", "sasso", "forbice"};*/

#include <stdio.h>
#include <semaphore.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <time.h>
#include <stdbool.h>

#define CARTA 0
#define SASSO 1
#define FORBICE 2
#define DA_LANCIARE -1
char *nomi_mosse[3] = {"carta", "sasso", "forbice"};

int nanosleep(const struct timespec *req, struct timespec *rem);
void pausetta(void)
{
    struct timespec t;
    t.tv_sec = 0;
    t.tv_nsec = (rand() % 10 + 1) * 1000000;
    nanosleep(&t, NULL);
}

struct partita_t
{
    pthread_mutex_t mutex;
    pthread_cond_t player1, player2, arbitro;
    int giocate[2];

} partita;

void initPartita(struct partita_t *partita)
{

    pthread_mutexattr_t mutex_attr;
    pthread_condattr_t cond_attr;

    pthread_mutexattr_init(&mutex_attr);
    pthread_condattr_init(&cond_attr);

    pthread_mutex_init(&partita->mutex, &mutex_attr);
    pthread_cond_init(&partita->player1, &cond_attr);
    pthread_cond_init(&partita->player2, &cond_attr);
    pthread_cond_init(&partita->arbitro, &cond_attr);

    pthread_mutexattr_destroy(&mutex_attr);
    pthread_condattr_destroy(&cond_attr);

    partita->giocate[0] = DA_LANCIARE;
    partita->giocate[1] = DA_LANCIARE;
}

int scelta_mossa()
{
    return rand() % 3;
}

void giocaPlayer1(struct partita_t *partita)
{
    pthread_mutex_lock(&partita->mutex);
    pthread_cond_wait(&partita->player1, &partita->mutex);
    // sceglie una mossa.
    int mossa = scelta_mossa();
    printf("Player1 sceglie %s\n", nomi_mosse[mossa]);
    partita->giocate[0] = mossa;
    pthread_cond_signal(&partita->arbitro);
    pthread_mutex_unlock(&partita->mutex);
}

void giocaPlayer2(struct partita_t *partita)
{
    pthread_mutex_lock(&partita->mutex);
    pthread_cond_wait(&partita->player2, &partita->mutex);
    // sceglie una mossa.
    int mossa = scelta_mossa();
    printf("Player2 sceglie %s\n", nomi_mosse[mossa]);
    partita->giocate[1] = mossa;
    pthread_cond_signal(&partita->arbitro);
    pthread_mutex_unlock(&partita->mutex);
}

void checkVincita(struct partita_t *partita)
{
    int giocata1 = partita->giocate[0];
    int giocata2 = partita->giocate[1];
    int vincitore = -1;

    if ((giocata1 == FORBICE && giocata2 == CARTA) || (giocata1 == CARTA && giocata2 == SASSO) || (giocata1 == SASSO && giocata2 == FORBICE))
    {
        vincitore = giocata1;
    }
    else if ((giocata2 == SASSO && giocata1 == FORBICE) || (giocata2 == CARTA && giocata1 == SASSO) || (giocata2 == FORBICE && giocata1 == CARTA))
    {
        vincitore = giocata2;
    }

    if (vincitore == giocata1)
    {
        printf("Ha vinto il giocatore n.1 con la mossa --> %s\n", nomi_mosse[vincitore]);
    }
    else if (vincitore == giocata2)
    {
        printf("Ha vinto il giocatore n.2 con la mossa --> %s\n", nomi_mosse[vincitore]);
    }
    else
    {
        printf("Pari merito con la mossa --> %s\n", nomi_mosse[partita->giocate[1]]);
    }
}

void InizioPartita(struct partita_t *partita)
{
    // printf("Chiamo player1");
    pthread_cond_signal(&partita->player1);
    // printf("Chiamo player2");
    pthread_cond_signal(&partita->player2);
}

void arbitra(struct partita_t *partita)
{
    // l'arbitro deve prendere il mutex.

    // l'arbitro si blocca solo mentre i due giocatori giocano.
    while (1)
    {
        // inizia la partita.
        InizioPartita(partita);
        pthread_mutex_lock(&partita->mutex);
        // si blocca, finchè i due giocatori non lo svegliano. La condizione di blocco è questa:
        while (partita->giocate[0] == DA_LANCIARE || partita->giocate[1] == DA_LANCIARE)
        {
            pthread_cond_wait(&partita->arbitro, &partita->mutex);
        }

        // se si arriva qui: è finita la partita. --> faccio il checkVincita in quanto arbitro.
        checkVincita(partita);

        // resetto per un'eventuale nuova partita.
        partita->giocate[0] = DA_LANCIARE;
        partita->giocate[1] = DA_LANCIARE;

        // attendo l'invio da tastiera
        // printf("Premi INVIO per una nuova partita\n");
        // char c = getchar();
        pthread_mutex_unlock(&partita->mutex);
    }
}

void *Player1(void *arg)
{

    for (;;)
    {

        giocaPlayer1(&partita);
    }
}

void *Player2(void *arg)
{

    for (;;)
    {

        giocaPlayer2(&partita);
    }
}

void *Arbitro(void *arg)
{

    for (;;)
    {
        arbitra(&partita);
    }
}

int main(int argc, char **argv)
{
    pthread_attr_t a;
    pthread_t p;

    /* inizializzo il sistema */
    initPartita(&partita);

    /* inizializzo i numeri casuali, usati nella funzione pausetta */
    srand(555);

    pthread_attr_init(&a);

    /* non ho voglia di scrivere 10000 volte join! */
    pthread_attr_setdetachstate(&a, PTHREAD_CREATE_DETACHED);

    pthread_create(&p, &a, Player1, NULL);
    pthread_create(&p, &a, Player2, NULL);
    pthread_create(&p, &a, Arbitro, NULL);

    pthread_attr_destroy(&a);

    sleep(5);

    return 0;
}