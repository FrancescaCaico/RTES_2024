/*
All’interno di un’applicazione, vari processi (ciclici) cooperano, secondo i criteri specificati nel seguito,
accedendo ad una risorsa condivisa R.
Alcuni fra questi processi, arrivati ad un certo punto della loro esecuzione, per procedere devono
controllare che si sia verificato un certo evento (che indicheremo come eventoA). In caso positivo
procedono senza bloccarsi altrimenti attendono che l’evento si verifichi.
Questo controllo viene effettuato accedendo alla risorsa R tramite la funzione testaA.
Altri processi sono preposti a segnalare l’occorrenza dell’evento invocando la funzione segnalaA.
Gli eventi segnalati sono però, per così dire, “consumabili”.
In particolare, se un processo, invocando
testaA verifica che eventoA si è già verificato, procede senza bloccarsi ma resetta l’evento, cioè, da quel momento in poi se un altro processo invoca testaA, questo si
deve bloccare in attesa che, tramite segnalaA l’evento sia di nuovo segnalato.
Se all’atto dell’invocazione di testaA l’evento non si è ancora verificato, o se verificato è già stato consumato, allora il processo
richiedente si blocca.
Viceversa, quando viene invocata segnalaA, se non ci sono processi in attesa
dell’evento, allora questa segnalazione resta disponibile per il primo processo che invochi testaA, se
viceversa uno o più processi sono in attesa dell’evento, tutti vengono riattivati e l’evento viene
consumato. Infine, se all’atto dell’invocazione di segnalaA e` ancora presente un precedente evento non
consumato, il processo segnalante si deve bloccare in attesa che il precedente evento venga consumato.
1- utilizzando il meccanismo semaforico implementare R con le due funzioni testaA e segnalaA.
2- Supponendo che i processi che segnalano l’evento siano P1, …., PN, e che ciascun processo
nell’invocare la funzione segnalaA passi il proprio nome (intero tra 1 e N) come parametro,
modificare la precedente soluzione imponendo una priorità tra questi processi quando devono
essere risvegliati dopo un blocco in modo tale che P1 abbia priorità su P2 ecc.
*/

/* NON SI CAPISCE LA CONSEGNA MI DISP.*/
#include <stdio.h>
#include <semaphore.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <time.h>
#include <stdbool.h>
#include <stdint.h>
#include <semaphore.h>
#include <time.h>

// ALCUNI SEGNALANO ALTRI ATTENDONO L'EVENTO --> LO SCEGLIAMO IN MODO RANDOMICO CHI È COSA

#define NTHREADS 5

// con semafori

struct x_t
{
    sem_t mutex; // mutex per accedere in modo mutuamente esclusivo

    sem_t b_on_event;       // semaforo su cui bloccarsi se l'eventoA non si è verificato e si aspetta la segnalazione
    sem_t b_on_consumption; // semaforo per bloccarsi se l'evento non è stato consumato.
    int waiting_event;      // quanti sono in attesa che si verifichi l'evento A

    bool evento_consumato; // se è stato consumato --> per la segnalazione
    bool evento;           // se è avvenuto l'evento

} x;

void initX(struct x_t *x)
{
    sem_init(&x->mutex, 0, 1);
    sem_init(&x->b_on_event, 0, 0);

    x->evento = x->evento_consumato = false;
    x->waiting_event = 0;
}
/*
    Verifica se l'evento eventoA si è già verificato e non è stato consumato:
        Se eventoA si è verificato ma non è stato consumato, il processo procede senza bloccarsi e resetta l'evento.
        Da questo momento in poi, se un altro processo chiama testaA, esso si bloccherà in attesa che l'evento sia segnalato nuovamente.
    Si blocca se l'evento non si è ancora verificato o se è già stato consumato:
        Se l'evento non si è ancora verificato oppure è già stato consumato da un altro processo, il processo richiedente si blocca in attesa che l'evento si verifichi e venga segnalato nuovamente.
*/
void testaA(struct x_t *x)
{

    sem_wait(&x->mutex);
    if (x->evento && !x->evento_consumato)
    {
        // resetto l'evento ma non mi fermo
        // non lo consuma?
        x->evento = 0;
    }
    else
    {
        sem_post(&x->mutex);
        sem_wait(&x->b_on_event); // qui mi svegliano i segnalanti
    }
    // una volta svegliato se sono qui devo controllare se è stato consumato altrimenti lo consumoà
    if (x->evento_consumato)
    {
        x->evento_consumato = 1;
        // devo segnalare i segnalanti
    }
}

void segnalaA(struct x_t *x)
{
    // ci sono più segnalanti
    sem_wait(&x->mutex);

    // una volta svegliato se sono qui
}