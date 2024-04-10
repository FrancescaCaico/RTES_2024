/*

La corsa – 14/12/2006
In uno stadio ci sono N corridori pronti a partire sulla pista dei 100 mt. Al via dell'arbitro, tutti i corridori
iniziano a correre. Dopo alcuni secondi tutti i corridori arrivano al traguardo. Quando l'ultimo corridore
raggiunge il traguardo l'arbitro annuncia il primo e l'ultimo classificato.
Il sistema è modellato con un thread per ogni corridore, ed un thread per l'arbitro. Il corridore arriva al punto di
partenza ed aspetta il via dell'arbitro. Quando l'arbitro da il via, il corridore corre ed arriva al traguardo.
L'arbitro arriva ed attende che tutti i corridori si siano posizionati. Dopo di che da il via alla gara, e si mette in
attesa dell'arrivo dell'ultimo corridore. Non appena l'ultimo corridore arriva, l'arbitro comunica il primo e
l'ultimo classificato della gara.
struct corsa_t { ... } corsa;
void init_corsa(struct corsa_t *s);
void corridore_attendivia(struct corsa_t *corsa, int numerocorridore);
void corridore_arrivo(struct corsa_t *corsa, int numerocorridore);
void arbitro_attendicorridori(struct corsa_t *corsa);
void arbitro_via(struct corsa_t *corsa);
void arbitro_risultato(struct corsa_t *corsa, int *primo, int *ultimo);
void *corridore(void *arg)
{
<vado sulla pista>
corridore_attendivia(struct corsa_t *corsa, int numerocorridore); // bloccante
<corro più veloce possibile>
corridore_arrivo(struct corsa_t *corsa, int numerocorridore); // non bloccante
<torno a casa>
}
void *arbitro(void *arg)
{
<vado sulla pista>
arbitro_attendicorridori(struct corsa_t *corsa); // bloccante
<pronti, attenti, ...>
arbitro_via(struct corsa_t *corsa); // non bloccante
<attendo che arrivino al termine>
arbitro_risultato(struct corsa_t *corsa, int *primo, int *ultimo); // bloccante
}
Modellare il sistema in linguaggio C con la libreria pthread nel seguente modo utilizzando:
[ ] semafori [ ] mutex + variabili condition
Scrivere il contenuto della struct corsa e le funzioni init_corsa, corridore_attendivia, corridore_arrivo,
arbitro_attendicorridori, arbitro_via, arbitro_risultato.

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
#include <semaphore.h>
#include <time.h>

#define NCORRIDORI 5
int first;
int last;

int nanosleep(const struct timespec *req, struct timespec *rem);

struct corsa_t
{
    pthread_mutex_t corsetta;

    // i corridori devono bloccarsi per attendere il via.
    pthread_cond_t corridori_via;

    // l'arbitro deve attendere la fine della corsa.
    pthread_cond_t arbitro;

    int corr_arrivati;
    int corr_al_via;
    int primo, ultimo;

} corsa;

void init_corsa(struct corsa_t *s)
{
    pthread_mutexattr_t mutexattr;
    pthread_condattr_t condattr;
    pthread_mutexattr_init(&mutexattr);
    pthread_condattr_init(&condattr);

    pthread_mutex_init(&corsa.corsetta, &mutexattr);
    pthread_cond_init(&corsa.corridori_via, &condattr);
    pthread_cond_init(&corsa.arbitro, &condattr);

    corsa.corr_al_via = corsa.corr_arrivati = 0;
    corsa.primo = corsa.ultimo = -1; // nessuno è primo o ultimo

    pthread_mutexattr_destroy(&mutexattr);
    pthread_condattr_destroy(&condattr);
}
void corridore_attendivia(struct corsa_t *corsa, int numerocorridore)
{
    pthread_mutex_lock(&corsa->corsetta);

    printf("Corridore %d> Attendo il via\n", numerocorridore);
    corsa->corr_al_via++;
    if (corsa->corr_al_via == NCORRIDORI)
    {
        printf("Corridore %d> Dico all'arbitro che siamo tutti al via\n", numerocorridore);
        // sono arrivati tutti i corridori, io che sono l'ultimo devo svegliare l'arbitro in attesa e poi mi blocco attendendo il via.
        pthread_cond_signal(&corsa->arbitro);
    }

    pthread_cond_wait(&corsa->corridori_via, &corsa->corsetta);
    printf("Corridore %d> Ricevuto il via\n", numerocorridore);
    pthread_mutex_unlock(&corsa->corsetta);
}
void corridore_arrivo(struct corsa_t *corsa, int numerocorridore)
{
    pthread_mutex_lock(&corsa->corsetta);

    printf("Corridore %d> Sono arrivato al traguardo\n", numerocorridore);
    corsa->corr_arrivati++;
    if (corsa->corr_arrivati == NCORRIDORI)
    {
        printf("Corridore %d> Sono l'ultimo arrivato, sveglio l'arbitro\n", numerocorridore);
        // sono arrivati tutti i corridori, io che sono l'ultimo devo svegliare l'arbitro in attesa e poi mi blocco attendendo il via.
        corsa->ultimo = numerocorridore;
        pthread_cond_signal(&corsa->arbitro);
    }
    else if (corsa->corr_arrivati == 1)
    {
        printf("Corridore %d> Sono il primo, attendo la fine dell'arbitro (io il mio l'ho fatto)\n", numerocorridore);
        corsa->primo = numerocorridore;
    }
    pthread_mutex_unlock(&corsa->corsetta);
}
void arbitro_attendicorridori(struct corsa_t *corsa)
{

    pthread_mutex_lock(&corsa->corsetta);
    while (corsa->corr_al_via != NCORRIDORI)
    {
        pthread_cond_wait(&corsa->arbitro, &corsa->corsetta);
    }
    // Sono arrivati tutti i corridori al traguardo.

    pthread_mutex_unlock(&corsa->corsetta);
}

void arbitro_via(struct corsa_t *corsa)
{
    pthread_mutex_lock(&corsa->corsetta);
    printf("**** VIA ****\n");
    pthread_cond_broadcast(&corsa->corridori_via);

    pthread_mutex_unlock(&corsa->corsetta);
}
void arbitro_risultato(struct corsa_t *corsa, int *primo, int *ultimo)
{
    pthread_mutex_lock(&corsa->corsetta);
    while (corsa->corr_arrivati != NCORRIDORI)
    {
        pthread_cond_wait(&corsa->arbitro, &corsa->corsetta);
    }
    // Sono arrivati tutti i corridori al traguardo.
    *primo = corsa->primo;
    *ultimo = corsa->ultimo;

    pthread_mutex_unlock(&corsa->corsetta);
    printf("Primo della gara --> Giocatore %d!\nUltimo della gara --> Giocatore %d\n**** FINE GARA ****\n", *primo, *ultimo);
}

void pausetta(int quanto)
{
    struct timespec t;
    t.tv_sec = 0;
    t.tv_nsec = (rand() % 100 + 1) * 1000000 + quanto;
    nanosleep(&t, NULL);
}

void *corridore(void *arg)
{
    int numerocorridore = (intptr_t)arg;
    pausetta(100000);

    corridore_attendivia(&corsa, numerocorridore); // bloccante
    pausetta(100000);

    corridore_arrivo(&corsa, numerocorridore); // non bloccante
    return NULL;
}
void *arbitro(void *arg)
{
    pausetta(100000);
    arbitro_attendicorridori(&corsa); // bloccante
    pausetta(100000);

    arbitro_via(&corsa); // non bloccante
    pausetta(100000);

    arbitro_risultato(&corsa, &first, &last); // bloccante
    return NULL;
}

int main()
{
    srand(time(NULL));

    init_corsa(&corsa);

    pthread_t corridori[NCORRIDORI];
    pthread_t arbitro_thread;

    // Creazione dei thread dei corridori
    for (intptr_t i = 0; i < NCORRIDORI; ++i)
    {
        pthread_create(&corridori[i], NULL, corridore, (void *)i);
    }

    // Creazione del thread dell'arbitro
    pthread_create(&arbitro_thread, NULL, arbitro, NULL);

    // Join dei thread dei corridori
    for (int i = 0; i < NCORRIDORI; ++i)
    {
        pthread_join(corridori[i], NULL);
    }

    // Join del thread dell'arbitro
    pthread_join(arbitro_thread, NULL);

    return 0;
}