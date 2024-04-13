#ifndef QUEUE_H
#define QUEUE_H

typedef int T;
// Definizione della struttura node_t per i nodi della lista
typedef struct Busta
{
    T data;             // Dato memorizzato nel nodo
    struct Busta *next; // Puntatore al prossimo nodo nella lista
} busta_t;

// Definizione della struttura Queue
typedef struct
{
    busta_t *front; // Puntatore al primo nodo della coda
    busta_t *rear;  // Puntatore all'ultimo nodo della coda
    int size;       // Dimensione corrente della coda
    int capacity;   // Capacità massima della coda
} Queue;

// Funzione per creare una nuova coda con capacità specificata
Queue *createQueue(int capacity);

// Funzione per verificare se la coda è piena
int isFull(Queue *queue);

// Funzione per verificare se la coda è vuota
int isEmpty(Queue *queue);

// Funzione per aggiungere un elemento alla coda
void enqueue(Queue *queue, int data);

// Funzione per rimuovere e restituire il primo elemento dalla coda
int dequeue(Queue *queue);

// Funzione per visualizzare gli elementi presenti nella coda
void display(Queue *queue);

// Funzione per deallocare la memoria allocata per la coda
void freeQueue(Queue *queue);

#endif /* QUEUE_H */
