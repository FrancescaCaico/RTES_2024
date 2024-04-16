/*

Esercizio 13
Alcuni processi mittenti inviano periodicamente messaggi di un tipo mes (che si suppone già definito),
a due processi riceventi (R1 ed R2).
I messaggi vengono inviati tramite una mailbox da realizzare per mezzo di un buffer circolare di N posizioni.
Tutti i messaggi inviati devono essere ricevuti da entrambe i processi riceventi e ciascun processo deve ricevere tutti
i messaggi in ordine FIFO (quindi una posizione della mailbox una volta riempita con un messaggio,
potrà essere resa libera solo dopo che tale messaggio sia stato ricevuto dai due processi R1 ed R2).
Chiaramente, un generico messaggio potrà essere ricevuto prima
da R1 e poi da R2 mentre un altro messaggio potrà essere ricevuto in ordine inverso a seconda dei rapporti
di velocità tra i processi.
a)utilizzando il meccanismo dei semafori, realizzare la precedente mailbox con le tre funzioni send, receive1 e
receive2 invocate, rispettivamente, da ciascun mittente e da R1 e R2.
b)ripetere la soluzione con il vincolo che ogni messaggio debba sempre essere ricevuto prima da R1 e poi da R2.*/

/* IMPLEMENTAZIONE BUFFER CIRCOLARE */
#include <stdio.h>
#include <stdlib.h>

#define BUFFER_SIZE 10

typedef struct
{
    int data[BUFFER_SIZE];
    int head;
    int tail;
    int count;
} CircularBuffer;

void initBuffer(CircularBuffer *buffer)
{
    buffer->head = 0;
    buffer->tail = 0;
    buffer->count = 0;
}

int isEmpty(CircularBuffer *buffer)
{
    return buffer->count == 0;
}

int isFull(CircularBuffer *buffer)
{
    return buffer->count == BUFFER_SIZE;
}

void enqueue(CircularBuffer *buffer, int value)
{
    if (isFull(buffer))
    {
        printf("Buffer pieno, impossibile inserire %d\n", value);
        return;
    }

    buffer->data[buffer->tail] = value;
    buffer->tail = (buffer->tail + 1) % BUFFER_SIZE;
    buffer->count++;
}

int dequeue(CircularBuffer *buffer)
{
    if (isEmpty(buffer))
    {
        printf("Buffer vuoto, impossibile estrarre\n");
        return -1; // Valore sentinella per indicare l'errore
    }

    int value = buffer->data[buffer->head];
    buffer->head = (buffer->head + 1) % BUFFER_SIZE;
    buffer->count--;

    return value;
}

void printBuffer(CircularBuffer *buffer)
{
    printf("Contenuto del buffer: ");
    if (isEmpty(buffer))
    {
        printf("Vuoto\n");
        return;
    }

    int i = buffer->head;
    while (i != buffer->tail)
    {
        printf("%d ", buffer->data[i]);
        i = (i + 1) % BUFFER_SIZE;
    }
    printf("\n");
}