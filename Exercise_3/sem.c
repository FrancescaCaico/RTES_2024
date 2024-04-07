/*In un sistema organizzato secondo il modello a memoria comune si vuole realizzare un meccanismo di
comunicazione tra processi che simula una mailbox a cui M diversi processi mittenti inviano messaggi di
un tipo T predefinito e da cui prelevano messaggi R diversi processi riceventi.
Per simulare tale meccanismo si definisce il tipo busta di cui si suppone di usare N istanze (costituenti un
pool di risorse equivalenti). Un gestore G alloca le buste appartenenti al pool ai processi mittenti i quali,
per inviare un messaggio eseguono il seguente algoritmo:
send (messaggio) => 1 - richiesta al gestore G di una busta vuota;
2 - inserimento nella busta del messaggio;
3 - accodamento della busta nella mailbox;
Analogamente ogni processo ricevente, per ricevere un messaggio, esegue il seguente algoritmo:
messaggio = receive() => 1 - estrazione della busta dalla mailbox;
2 - estrazione del messaggio dalla busta;
3 - rilascio della busta vuota al gestore
Realizzare il precedente meccanismo utilizzando i semafori e garantendo che la receive sia bloccante
quando nella mailbox non ci sono buste, e che la send sia bloccante quando non ci sono piu` buste vuote
disponibili. Indicare, in particolare, come viene definita la busta, il codice del gestore e della mailbox, il
codice delle due funzioni send e receive.
Per garantire la ricezione FIFO dei messaggi, organizzare le buste nella mailbox mediante una coda
concatenata. Il gestore alloca le buste vuote ai processi mittenti adottando una politica prioritaria in base
ad un parametro priorita` che ciascun processo indica nel momento in cui chiede una busta vuota al
gestore. La priorita` che ogni processo indica puo` essere 0 (priorita` massima, 1 (priorita` intermedia)
oppure 2 (priorita` minima) Per richieste specificate con uguale priorita` viene seguita la politica FIFO.
Si puo` supporre che le code semaforiche siano gestite FIFO*/

#include <stdio.h>
#include <semaphore.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <time.h>
#include <stdbool.h>

#define M 11
#define R 9
#define N 6
#define NESSUNO -1

int nanosleep(const struct timespec *req, struct timespec *rem);
void pausetta(void)
{
    struct timespec t;
    t.tv_sec = 0;
    t.tv_nsec = (rand() % 10 + 1) * 1000000;
    nanosleep(&t, NULL);
}

typedef int T;
struct semaforo_privato
{
    sem_t sem;
    int blocked;
    int active;
} sem_priv;

struct busta_t
{
    T message;
    bool piena;
} busta;

struct gestore_t
{
    sem_t mutex;
    struct semaforo_privato mittenti[3];
    struct semaforo_privato riceventi;

    // si deve implementare la coda, il gestore deve sapere a quale messaggio è arrivato

    int tail, head, free;

    // deve anche sapere per ogni busta qual è la successiva da immettere o estrarre
    int prossima[N];

} gestore;

struct mailbox_t
{
    struct busta_t buste[N];
    struct gestore_t gestore;

} mailbox;

void initGestore(struct gestore_t *g)
{
    sem_init(&g->mutex, 0, 1);
    sem_init(&g->mittenti[0].sem, 0, 0);
    sem_init(&g->mittenti[1].sem, 0, 0);
    sem_init(&g->mittenti[2].sem, 0, 0);
    sem_init(&g->riceventi.sem, 0, 0);

    // nessuno ha ancora immesso o estratto le buste. --> tail punto di estrazione, head punto di inserimento --> Garantisce fifo
    g->tail = g->head = -1;
    // la prima locazione è libera
    g->free = 0;

    for (int i = 0; i < N - 1; ++i)
    {
        g->prossima[i] = i + 1;
    }
    g->prossima[N - 1] = -1;
}

void initMailbox(struct mailbox_t *mailbox)
{
    initGestore(&mailbox->gestore);
}
int richiestaBusta(struct gestore_t *g, int prio)
{
    int blocco, indice_libera;
    sem_wait(&g->mutex);

    // ho preso il mutex. vediamo se ci sono buste libere rispetto alla mia priorità e se non c'è qualcuno già in attesa

    if (prio == 0)
    {
        // vediamo se nella coda c'è già qualcuno in attesa.
        blocco = g->mittenti[0].blocked;
    }
    else if (prio == 1)
    {
        blocco = g->mittenti[1].blocked;
    }
    else if (prio == 2)
    {
        blocco = g->mittenti[2].blocked;
    }

    if (blocco || g->free == -1)
    {
        // mi devo bloccare.
        g->mittenti[prio].blocked++;
        sem_post(&g->mutex);
        sem_wait(&g->mittenti[prio].sem);
        g->mittenti[prio].blocked--;
    }

    // a questo punto se mi hanno svegliato c'è per la mia coda un posto libero in cui posso inserire la busta
    printf("busta vuota %d\n", g->free);
    indice_libera = g->free;
    g->free = g->prossima[g->free];

    sem_post(&g->mutex);
    return indice_libera;
}

void accodaMessaggio(struct gestore_t *g, int busta_da_inserire)
{
    sem_wait(&g->mutex);

    // devo controllare la head e la tail e modificarle.
    // se non ho ancora accodato --> la head è nulla e posso inserire.

    if (g->head == -1)
    {
        g->head = busta_da_inserire;
    }
    else
    {
        // la mia busta va inserita, e deve essere estratta dopo ql'ultima estrazione x fifo.
        g->prossima[g->tail] = busta_da_inserire;
    }
    g->tail = busta_da_inserire; // la prossima da estrarre.

    if (g->riceventi.blocked)
    {
        // li sveglio perchè ho inserito un nuovo messaggio
        sem_post(&g->riceventi.sem);
    }
    else
    {
        // rilascio il mutex e amen
        sem_post(&g->mutex);
    }
}

void send(T message, struct mailbox_t *mailbox, int prio)
{
    int indice_libero = richiestaBusta(&mailbox->gestore, prio);

    // inserimento del messaggio nella busta
    fprintf(stderr, "  s %8d\n", message);
    mailbox->buste[indice_libero].message = message;
    // accodamento nella mailbox
    accodaMessaggio(&mailbox->gestore, indice_libero);
}

int estrai_busta(struct gestore_t *g)
{

    sem_wait(&g->mutex);

    // estraggo una busta solo se la mailbox contiene una busta altrimenti mi ritrovo bloccato.
    if (g->head == -1)
    {
        // non ci sono ancora buste inserite
        g->riceventi.blocked++;
        sem_post(&g->mutex);
        sem_wait(&g->riceventi.sem);
        g->riceventi.blocked--;
    }

    // ho una busta da estrarre.
    int indice_busta_estratta = g->head;
    g->head = g->prossima[g->head];

    sem_post(&g->mutex);
    return indice_busta_estratta;
}

void rilascioBustaVuota(struct gestore_t *g, int busta_da_rilasciare)
{
    sem_wait(&g->mutex);
    g->prossima[busta_da_rilasciare] = g->free;
    g->free = busta_da_rilasciare;

    // devo svegliare chi è in attesa, in modo prioritario tra i vari mittenti
    if (g->mittenti[0].blocked)
    {
        sem_post(&g->mittenti[0].sem);
    }
    else if (g->mittenti[1].blocked)
    {
        sem_post(&g->mittenti[1].sem);
    }
    else if (g->mittenti[2].blocked)
    {
        sem_post(&g->mittenti[2].sem);
    }
    else
    {

        sem_post(&g->mutex);
    }
}

T receive(struct mailbox_t *mailbox)
{
    int busta_ricevuta = estrai_busta(&mailbox->gestore);
    // prendo il dato inserito nella mailbox contenente il messaggio T
    T dato = mailbox->buste[busta_ricevuta].message;
    fprintf(stderr, "   r %7d\n", dato);
    rilascioBustaVuota(&mailbox->gestore, busta_ricevuta);
    return dato;
}

int cont = 0;
/* i thread */

void *mittente(void *arg)
{
    int p = (int)arg; // Priorita'
    T i;

    for (;;)
    {
        i = ++cont;
        fprintf(stderr, "S %4d%6d\n", p, i);
        send(cont, &mailbox, p);
        pausetta();
    }
    return 0;
}

void *ricevente(void *arg)
{
    T i;

    for (;;)
    {
        i = receive(&mailbox);
        fprintf(stderr, " R%10d\n", i);
        pausetta();
    }
    return 0;
}

/* la creazione dei thread */

int main()
{
    pthread_attr_t a;
    pthread_t p;
    int i;

    /* inizializzo il mio sistema */
    initMailbox(&mailbox);

    /* inizializzo i numeri casuali, usati nella funzione pausetta */
    srand(555);

    pthread_attr_init(&a);

    /* non ho voglia di scrivere 10000 volte join! */
    pthread_attr_setdetachstate(&a, PTHREAD_CREATE_DETACHED);

    for (i = 0; i < M; i++)
        pthread_create(&p, &a, mittente, (void *)(rand() % 3));

    for (i = 0; i < R; i++)
        pthread_create(&p, &a, ricevente, NULL);

    pthread_attr_destroy(&a);

    /* aspetto 10 secondi prima di terminare tutti quanti */
    sleep(3);

    return 0;
}
