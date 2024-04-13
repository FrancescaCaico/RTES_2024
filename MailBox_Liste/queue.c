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
