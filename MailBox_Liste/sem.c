#include "queue.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "queue.h"

// Funzione per creare una nuova coda con capacità specificata
Queue *createQueue(int capacity)
{
    Queue *queue = (Queue *)malloc(sizeof(Queue));
    if (queue == NULL)
    {
        fprintf(stderr, "Errore: impossibile allocare memoria per la coda\n");
        exit(EXIT_FAILURE);
    }
    queue->front = NULL;
    queue->rear = NULL;
    queue->size = 0;
    queue->capacity = capacity;
    return queue;
}

// Funzione per verificare se la coda è piena
int isFull(Queue *queue)
{
    return (queue->size >= queue->capacity);
}

// Funzione per verificare se la coda è vuota
int isEmpty(Queue *queue)
{
    return (queue->front == NULL);
}

// Funzione per aggiungere un elemento alla coda
void enqueue(Queue *queue, int data)
{
    // Verifica se la coda è piena
    if (isFull(queue))
    {
        fprintf(stderr, "Errore: la coda è piena, impossibile aggiungere nuovi elementi\n");
        return;
    }

    // Creazione del nuovo nodo
    busta_t *newNode = (busta_t *)malloc(sizeof(busta_t));
    if (newNode == NULL)
    {
        fprintf(stderr, "Errore: impossibile allocare memoria per il nuovo nodo\n");
        exit(EXIT_FAILURE);
    }
    newNode->data = data;
    newNode->next = NULL;

    // Aggiunta del nuovo nodo alla coda
    if (isEmpty(queue))
    {
        queue->front = newNode;
    }
    else
    {
        queue->rear->next = newNode;
    }
    queue->rear = newNode;
    queue->size++;
}

// Funzione per rimuovere e restituire il primo elemento dalla coda
int dequeue(Queue *queue)
{
    if (isEmpty(queue))
    {
        fprintf(stderr, "Errore: la coda è vuota, impossibile rimuovere elementi\n");
        exit(EXIT_FAILURE);
    }
    // Rimozione del primo nodo dalla coda
    busta_t *removedNode = queue->front;
    int data = removedNode->data;
    queue->front = queue->front->next;
    free(removedNode);
    // Se la coda diventa vuota, aggiorniamo anche il puntatore rear
    if (queue->front == NULL)
    {
        queue->rear = NULL;
    }
    queue->size--;
    return data;
}

// Funzione per visualizzare gli elementi presenti nella coda
void display(Queue *queue)
{
    if (isEmpty(queue))
    {
        printf("La coda è vuota\n");
        return;
    }
    printf("Elementi nella coda: ");
    busta_t *current = queue->front;
    while (current != NULL)
    {
        printf("%d ", current->data);
        current = current->next;
    }
    printf("\n");
}

// Funzione per deallocare la memoria allocata per la coda
void freeQueue(Queue *queue)
{
    while (!isEmpty(queue))
    {
        dequeue(queue);
    }
    free(queue);
}

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

struct gestore_t
{
    sem_t mutex;
    struct semaforo_privato mittenti[3];
    struct semaforo_privato riceventi;

} gestore;

struct mailbox_t
{
    Queue *buste;
    struct gestore_t gestore;

} mailbox;

void initGestore(struct gestore_t *g)
{
    sem_init(&g->mutex, 0, 1);
    for (int i = 0; i < 3; i++)
    {
        g->mittenti[i].active = 0;
        g->mittenti[i].blocked = 0;
        sem_init(&g->mittenti[i].sem, 0, 0);
    }
}

void initMailbox(struct mailbox_t *mailbox)
{
    mailbox->buste = createQueue(5);
    initGestore(&mailbox->gestore);
}

void richiestaBusta(struct gestore_t *g, int prio)
{
    // quando si blocca il mittente? Quando non ci sono buste vuote e non c'è qualcuno con una priorità più elevata in attesa
    sem_wait(&g->mutex);

    bool blocco_mittente;

    if (g->mittenti[0].blocked)
    {

        blocco_mittente = 1;
    }
    else if (g->mittenti[0].blocked || g->mittenti[1].blocked)
    {
        blocco_mittente = 1;
    }
    else if (g->mittenti[0].blocked || g->mittenti[1].blocked || g->mittenti[2].blocked)
    {
        blocco_mittente = 1;
    }

    if (blocco_mittente || isFull(mailbox.buste))
    {
        // se c'è già qualcuno in attesa oppure la coda è piena mi blocco sulla mia coda prioritaria.
        g->mittenti[prio].blocked++;
        sem_post(&g->mutex);
        sem_wait(&g->mittenti[prio].sem); // al risveglio mi verrà passato il mutex
    }

    // chiediamo una busta vuota.

    sem_post(&g->mutex);
}

void accodaMessaggio(struct gestore_t *g, T message)
{
    sem_wait(&g->mutex);
    enqueue(mailbox.buste, message);
    if (g->riceventi.blocked)
    {
        printf("Mittente %ld > sveglio i riceventi bloccati\n", pthread_self());
        sem_post(&g->riceventi.sem);
    }
    else
    {
        sem_post(&g->mutex);
    }
}

void send(T message, struct mailbox_t *mailbox, int prio)
{

    richiestaBusta(&mailbox->gestore, prio);
    printf("Mittente %ld > Inserisco il messaggio\n", pthread_self());
    accodaMessaggio(&mailbox->gestore, message);
    printf("Mittente %ld > Finito\n", pthread_self());
    pausetta();
}

int estrai_busta(struct gestore_t *g)
{
    sem_wait(&g->mutex);
    if (isEmpty(mailbox.buste))
    {
        g->riceventi.blocked++;
        sem_post(&g->mutex);
        sem_wait(&g->riceventi.sem);
    }

    printf("Ricevente %ld > Prendo una busta con un messaggio\n", pthread_self());
    T messaggio_busta = dequeue(mailbox.buste);
    sem_post(&g->mutex);
    return messaggio_busta;
}

void rilascioBustaVuota(struct gestore_t *g)
{
    sem_wait(&g->mutex);
    printf("Ricevente %ld > Ho lasciato la busta\n", pthread_self());
    if (&g->mittenti[0].blocked)
    {
        g->mittenti[0].blocked--;
        sem_post(&g->mittenti[0].sem);
    }
    else if (&g->mittenti[1].blocked)
    {
        g->mittenti[1].blocked--;
        sem_post(&g->mittenti[1].sem);
    }
    else if (&g->mittenti[2].blocked)
    {
        g->mittenti[2].blocked--;
        sem_post(&g->mittenti[2].sem);
    }
    else
    {
        sem_post(&g->mutex);
    }
}

T receive(struct mailbox_t *mailbox)
{

    return estrai_busta(&mailbox->gestore);
}

int cont = 0;
/* i thread */

void *mittente(void *arg)
{
    int p = (intptr_t)arg; // Priorita'
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
    intptr_t prior;
    for (i = 0; i < M; i++)
        prior = rand() % 3;
    pthread_create(&p, &a, mittente, (void *)prior);

    for (i = 0; i < R; i++)
        pthread_create(&p, &a, ricevente, NULL);

    pthread_attr_destroy(&a);

    /* aspetto 10 secondi prima di terminare tutti quanti */
    sleep(10);

    return 0;
}
