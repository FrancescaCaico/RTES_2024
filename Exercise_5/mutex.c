/*In un programma multithread, ogni thread esegue il seguente codice:
void *thread(void *arg)
{
int voto = rand()%2;
vota(voto);
if (voto == risultato()) printf("Ho vinto!\n");
else printf("Ho perso!\n");
pthread_exit(0);
}
cioe' ogni thread:
-esprime un voto, che puo' essere 0 o 1, invocando la funzione vota(), la quale registra il voto in una
struttura dati condivisa che per comodita' chiameremo "urna";
-aspetta l'esito della votazione invocando la funzione risultato(), la quale controlla l'urna e ritorna 0 o 1
a seconda che ci sia una maggioranza di voti 0 oppure di voti 1.
-se l'esito della votazione e' uguale al proprio voto, stampa a video la stringa "Ho vinto", altrimenti
stampa la stringa "Ho perso";
Supponiamo che ci siano un numero dispari di threads nel sistema. Il candidato deve implementare la
struttura dati
struct {
...
} urna;
e le funzioni:
void vota(int v);
int risultato(void);
in modo che i thread si comportino come segue:
- Se l'esito della votazione non puo' ancora essere stabilito, la funzione risultato() deve bloccare il thread
chiamante. Non appena l'esito e' "sicuro" (ovvero almeno la meta' piu' uno dei threads ha votato 0,
oppure almeno la meta' piu' uno dei threads ha votato 1) il thread viene sbloccato e la funzione
risultato() ritorna l'esito della votazione. I thread vengono sbloccati il piu' presto possibile, quindi
anche prima che abbiano votato tutti.
Utilizzare i costrutti pthread_mutex_xxx e pthread_cond_xxx visti a lezione.*/

#include <stdio.h>
#include <semaphore.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <time.h>
#include <stdbool.h>

#define ELETTORI 7

struct urna_t
{
    pthread_mutex_t mutex;
    pthread_cond_t turno;
    pthread_cond_t scrutinio;

    int blocked, votanti, voti0, voti1, vittoria;
} urna;

void initUrna(struct urna_t *urna)
{
    pthread_mutexattr_t mutex_attr;
    pthread_condattr_t cond_attr;

    pthread_mutexattr_init(&mutex_attr);
    pthread_condattr_init(&cond_attr);

    pthread_mutex_init(&urna->mutex, &mutex_attr);
    pthread_cond_init(&urna->scrutinio, &cond_attr);
    pthread_cond_init(&urna->turno, &cond_attr);

    pthread_mutexattr_destroy(&mutex_attr);
    pthread_condattr_destroy(&cond_attr);

    urna->blocked = 0;
    urna->votanti = 0;
    urna->voti0 = 0;
    urna->voti1 = 0;
    urna->vittoria = -1;
}

int risultato(void)
{
    int risultato;
    pthread_mutex_lock(&urna.mutex);

    // se i voti sono già ben definiti sveglio tutti
    if ((urna.voti1 == (ELETTORI / 2) + 1))
    {
        // HA VINTO LO 0 SENZA CONTARE I VOTI 1
        // SVEGLIO TUTTI I BLOCCATI
        urna.vittoria = 1;
        pthread_cond_broadcast(&urna.scrutinio);
    }
    else if ((urna.voti0 == (ELETTORI / 2) + 1))
    {
        // HA VINTO LO 0 SENZA CONTARE I VOTI 1
        // SVEGLIO TUTTI I BLOCCATI SULLO SCRUTINIO.
        urna.vittoria = 0;
        pthread_cond_broadcast(&urna.scrutinio);
    }
    else if ((urna.voti0 + urna.voti1) != ELETTORI)
    {

        // mi blocco ovviamente.
        printf("Mi blocco perchè non hanno finito di votare\n");
        pthread_cond_wait(&urna.scrutinio, &urna.mutex);
    }

    // se sono l'ultimo che ha votato e i voti sono pari sveglio tutti.
    else if ((urna.voti0 + urna.voti1) == ELETTORI)
    {
        if (urna.voti0 > urna.voti1)
        {
            urna.vittoria = 0;
        }
        else
        {
            urna.vittoria = 1;
        }
        pthread_cond_broadcast(&urna.scrutinio);
    }

    // se non sono ancora definiti chi ha vinto --> mi blocco
    risultato = urna.vittoria;
    printf("0: %d, 1: %d\n", urna.voti0, urna.voti1);
    pthread_mutex_unlock(&urna.mutex);
    return risultato;
}

void vota(int v)
{
    pthread_mutex_lock(&urna.mutex);

    while (urna.votanti)
    {
        urna.blocked++;
        pthread_cond_wait(&urna.turno, &urna.mutex);
        urna.blocked--;
    }

    // posso votare.
    if (urna.vittoria == -1)
    {
        printf("NON HANNO VOTATO ANCORA.\n");
        urna.votanti++;
        if (v)
        {
            urna.voti1++;
        }
        else if (!v)
        {
            urna.voti0++;
        }
        pthread_cond_signal(&urna.turno);
        urna.votanti--;
    }
    else
    {
        printf("HANNO VOTATO GIA' PRIMA CHE IO VOTASSI.\n");

        pthread_cond_broadcast(&urna.turno);
    }
    // lascio a qualcun'altro la votazione

    pthread_mutex_unlock(&urna.mutex);
}

void *thread(void *arg)
{
    int thread_id = pthread_self();
    int voto = rand() % 2;
    printf("thread %d sta andando a votare %d\n", thread_id, voto);
    vota(voto);
    if (voto == risultato())
        printf("thread %d --> Ho vinto con %d!\n", thread_id, voto);
    else
        printf("thread %d --> Ho perso con %d!\n", thread_id, voto);

    pthread_exit(0);
}

int main(void)
{

    srand(time(NULL));

    pthread_attr_t a;
    pthread_t threads[ELETTORI];

    pthread_attr_init(&a);

    initUrna(&urna);

    for (int i = 0; i < ELETTORI; i++)
    {

        pthread_create(&threads[i], &a, thread, NULL);
    }

    for (int i = 0; i < ELETTORI; i++)
    {

        int *retval;
        pthread_join(threads[i], (void **)&retval);
    }

    return 0;
}