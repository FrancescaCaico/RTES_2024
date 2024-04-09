
/*
Utilizzando i semafori, realizzare un meccanismo di comunicazione fra 5 processi mittenti ciclici (M0,
M1, …, M4) e un processo ricevente ciclico R. Il tipo di dati (messaggi) che i processi si scambiano è
costituito da un array di 5 elementi di tipo T. Il buffer di comunicazione può contenere un unico
messaggio (ovviamente costituito da un array di 5 elementi di tipo T). All’interno di ogni suo ciclo il
processo R può ricevere il messaggio quando questo è pronto nel buffer. All’interno di ogni suo ciclo il
processo mittente Mi (i=0, 1, .., 4) invia un valore di tipo T che va riempire l’elemento di indice i del
buffer. Quando tutti gli elementi del buffer contengono un valore inviato dal corrispondente mittente (tutti
valori inviati dai mittenti all’interno dello stesso ciclo della loro esecuzione) il buffer è pronto per essere
letto dal processo R.
1) scrivere il codice delle funzioni send eseguite da ciascun mittente, della funzione receive eseguita dal
ricevente e dettagliare la struttura dati del meccanismo di comunicazione. Per quanto riguarda i
semafori indicare, per ciascuno di essi, lo scopo per cui vengono usati e, nell’ipotesi che questi siano
semafori di mutua esclusione, giustificarne la necessità.
2) indicare come dovrebbe essere modificata la precedente soluzione se i processi mittenti fossero
costituiti da due gruppi di 5 processi ciascuno MA0, MA1,..,MA4 e MB0, MB1,…,MB4 con il
vincolo che, se uno dei processi mittenti di un gruppo, durante uno dei propri cicli, riesce per primo a
inserire il proprio dato nel buffer, allora il buffer deve essere riempito con i soli dati provenienti dai
processi di quel gruppo.
*/

#include <stdio.h>
#include <semaphore.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <time.h>
#include <stdbool.h>
#include <stdint.h>

#include <time.h>

int nanosleep(const struct timespec *req, struct timespec *rem);
void pausetta(void)
{
    struct timespec t;
    t.tv_sec = 0;
    t.tv_nsec = (rand() % 10 + 1) * 10000000;
    nanosleep(&t, NULL);
}
#define NTHREADS 5
#define SIZE_BUFFER 5

typedef int T;

// il ricevente legge solo se il buffer è completamente pieno.

struct comunicazione_t
{
    // serve un mutex per accedere in modo mutuamente esclusivo al buffer
    sem_t mutex;

    // serve un semaforo per permettere ad ogni processo di scrivere sulla propria casella.
    sem_t mittenti[NTHREADS];
    sem_t destinatario;         // il semaforo per svegliare il ricevente e poi per bloccarsi
    int num_m_attesa[NTHREADS]; // vediamo in quenti sono in attesa per inserire il messaggio nella propria casella

    int messaggi_inviati;   // se i messaggi sono SIZE_BUFFER si deve leggere il messaggio
    int ricevente_bloccato; // booleani ricevente è in attesa
    int ricevente_attivo;   // booleano ricevente attivo
    T buffer[NTHREADS];

} com;

void initCom(struct comunicazione_t *com)
{
    sem_init(&com->mutex, 0, 1);
    sem_init(&com->destinatario, 0, 0);

    for (int i = 0; i < SIZE_BUFFER; i++)
    {
        sem_init(&com->mittenti[i], 0, 0);
        com->num_m_attesa[i] = 0;
        com->buffer[i] = -1;
    }

    com->messaggi_inviati = 0;
    com->ricevente_attivo = 0;
    com->ricevente_bloccato = 0;
}

void inserisciMessaggio(struct comunicazione_t *com, int index_thread)
{
    // come mittente devo, prima di inserire il messaggio entrare nella struttura
    sem_wait(&com->mutex);

    // se sono entrato, mi blocco se --> ho già immesso il mio messaggio o se il ricevente è attivo
    if (com->ricevente_attivo || com->buffer[index_thread] != -1)
    {
        // devo bloccarmi sul mio semaforo personale e incrementare i bloccati sul mio indice
        com->num_m_attesa[index_thread]++;
    }
    else
    {
        // posso scrivere il mio messaggio --> faccio la post previa
        sem_post(&com->mittenti[index_thread]);
    }

    sem_post(&com->mutex);
    sem_wait(&com->mittenti[index_thread]);
}

void inviaMessaggio(struct comunicazione_t *com, T message, int index_thread)
{
    // come mittente devo, prima di inserire il messaggio entrare nella struttura
    sem_wait(&com->mutex);

    com->buffer[index_thread] = message;
    com->messaggi_inviati++;

    sem_post(&com->mutex);
}

void rilasciaBuffer(struct comunicazione_t *com, int index_thread)
{
    // come mittente devo, prima di inserire il messaggio entrare nella struttura
    sem_wait(&com->mutex);

    // chi devo svegliare?
    if (com->messaggi_inviati == SIZE_BUFFER)
    {
        /* tutti hanno inserito il loro messaggio*/
        // devo svegliare il receiver.
        com->ricevente_attivo = 1;
        sem_post(&com->destinatario);
    }

    sem_post(&com->mutex);
}

void leggiBuffer(struct comunicazione_t *com)
{

    printf("Dest sta prendendo il mutex\n");
    sem_wait(&com->mutex);
    printf("Dest ha preso il mutex\n");

    if (com->messaggi_inviati == SIZE_BUFFER)
    {
        com->ricevente_attivo = 1;
        sem_post(&com->destinatario);
    }
    else
    {
        // mi blocco perchè il buffer non è completo
        printf("********************Il ricevente si bloccherà**********************\n");
        com->ricevente_bloccato = 1;
    }

    sem_post(&com->mutex);
    printf("Dest ha rilasciato il mutex\n");
    sem_wait(&com->destinatario);
    printf("********************Il ricevente è stato svegliato **********************\n");
}

void receive(struct comunicazione_t *com)
{
    printf("Dest sta per leggere i messaggi...\n");
    for (int i = 0; i < 5; ++i)
    {
        printf("%d> Ricevuto messaggio %d\n", i, com->buffer[i]);
    }
}

void svuotaBuffer(struct comunicazione_t *com)
{
    sem_wait(&com->mutex);
    printf("Dest sta prendendo il mutex per svuotare il buffer\n");

    for (int i = 0; i < NTHREADS; i++)
    {

        com->messaggi_inviati--;
        com->buffer[i] = -1;
        sem_post(&com->mittenti[i]);
    }

    sem_post(&com->mutex);
}

void *mittente(void *arg)
{
    int index = *((int *)arg);
    int i = 5;
    while (i > 0)
    {
        i--;
        // Genera un valore casuale
        printf("--- Inizio Mittente > %d ---\n", index);
        int valore = rand() % 100;
        inserisciMessaggio(&com, index);
        printf("--- Inizio SEND Mittente > %d ---\n", index);
        inviaMessaggio(&com, valore, index);
        rilasciaBuffer(&com, index);
        printf("Mittente %d ha inviato il valore %d\n", index, valore);
        pausetta();
    }
    return NULL;
}

void *ricevente(void *arg)
{
    int i = 5;
    while (i > 0)
    {
        // Genera un valore casuale
        i--;
        printf("---Inizio Destinatario ---\n");
        leggiBuffer(&com);
        receive(&com);
        svuotaBuffer(&com);
        printf("---Fine destinatario ---\n");
        pausetta();
    }
    return NULL;
}

int main()
{
    pthread_t thread_mittenti[5];
    pthread_t thread_ricevente;
    int index[5];

    initCom(&com);

    // Crea i thread per i mittenti
    for (int i = 0; i < NTHREADS; ++i)
    {
        index[i] = i;
        pthread_create(&thread_mittenti[i], NULL, mittente, &index[i]);
    }

    // Crea il thread per il ricevente
    pthread_create(&thread_ricevente, NULL, ricevente, NULL);

    // Attendi la terminazione dei thread mittenti
    for (int i = 0; i < 5; ++i)
    {
        pthread_join(thread_mittenti[i], NULL);
    }

    // Attendi la terminazione del thread ricevente
    pthread_join(thread_ricevente, NULL);

    return 0;
}