#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>

/**
 * STRUTTURA DATI
 *
 * Condivisa e utilizzata per gestione concorrenza e sincronizzazione
 **/
struct my_resource
{
    pthread_mutex_t mutex;
    pthread_cond_t cond;

    int contatore_cond;
} resource;

/**
 * INIZIALIZZAZIONE
 **/
void inizializza(struct my_resource *r)
{
    pthread_mutexattr_t mutexattr;
    pthread_condattr_t condattr;

    pthread_mutexattr_init(&mutexattr);
    pthread_condattr_init(&condattr);

    pthread_mutex_init(&r->mutex, &mutexattr);
    pthread_cond_init(&r->cond, &condattr);
    r->contatore_cond = 0;

    /*OVVIAMENTE POSSONO ESSERCI ALTRE COSE DA INIZIALIZZARE*/

    pthread_condattr_destroy(&condattr);
    pthread_mutexattr_destroy(&mutexattr);
}

/**
 * SOLUZIONE 1
 *
 * Prologo (post previa).
 **/
void prologo1(struct my_resource *r)
{

    pthread_mutex_lock(&r->mutex);
    // MI DEVO CHIEDERE QUANDO MI BLOCCO

    while ("<LA CONDIZIONE NON E' VERIFICATA>")
    {
        // AGGIORNO IL CONTATORE DELLA CODA
        r->contatore_cond++;
        pthread_cond_wait(&r->cond, &r->mutex); // la cond wait rilascia e riprende il mutex automaticamente
        r->contatore_cond--;
        // ri-verifico la condizione altrimenti mi riblocco
    }

    // QUI SI E' VERIFICATA LA CONDIZIONE E CONTINUO...

    pthread_mutex_unlock(&r->mutex);
}

/**
 * SOLUZIONE 1
 *
 * Epilogo. --> DEVO LASCIARE LA RISORSA E CAPIRE CHI SVEGLIARE.
 **/
void epilogo1(struct my_resource *r)
{
    // CHI DEVO SVEGLIARE?
    pthread_mutex_lock(&r->mutex);

    "<DEALLOCO RISORSA>"; // eventualmente decrementando un contatore
    if ("<POSSO SVEGLIARE QUALCUNO>")
    {
        pthread_cond_signal(&r->cond);

        // se devo sveglieare tutti -->
        pthread_cond_broadcast(&r->cond);
    }

    pthread_mutex_unlock(&r->mutex);
}
