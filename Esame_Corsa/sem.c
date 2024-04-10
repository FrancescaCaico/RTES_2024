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

void pausetta(int quanto)
{
    struct timespec t;
    t.tv_sec = 0;
    t.tv_nsec = (rand() % 100 + 1) * 1000000 + quanto;
    nanosleep(&t, NULL);
}
struct corsa_t
{
    sem_t corsetta;

    // i corridori devono bloccarsi per attendere il via.
    sem_t corridori;

    // l'arbitro deve attendere la fine della corsa.
    sem_t arbitro;

    int corr_arrivati;
    int corr_al_via;
    int primo, ultimo;

} corsa;

void init_corsa(struct corsa_t *s)
{
    sem_init(&corsa.corsetta, 0, 1);
    sem_init(&corsa.corridori, 0, 0);

    sem_init(&corsa.arbitro, 0, 0);

    corsa.corr_al_via = corsa.corr_arrivati = 0;
    corsa.primo = -1;
    corsa.ultimo = -1;
}
void corridore_attendivia(struct corsa_t *corsa, int numerocorridore)
{
    sem_wait(&corsa->corsetta);

    printf("Corridore %d> Attendo il via\n", numerocorridore);
    corsa->corr_al_via++;
    if (corsa->corr_al_via == NCORRIDORI)
    {
        printf("Corridore %d> Dico all'arbitro che siamo tutti al via\n", numerocorridore);
        // sono arrivati tutti i corridori, io che sono l'ultimo devo svegliare l'arbitro in attesa e poi mi blocco attendendo il via.
        sem_post(&corsa->arbitro);
    }
    sem_post(&corsa->corsetta);
    sem_wait(&corsa->corridori);
    printf("Corridore %d> Ricevuto il via\n", numerocorridore);
}
void corridore_arrivo(struct corsa_t *corsa, int numerocorridore)
{
    sem_wait(&corsa->corsetta);

    printf("Corridore %d> Sono arrivato al traguardo\n", numerocorridore);
    corsa->corr_arrivati++;
    if (corsa->corr_arrivati == NCORRIDORI)
    {
        printf("Corridore %d> Sono l'ultimo arrivato, sveglio l'arbitro\n", numerocorridore);
        // sono arrivati tutti i corridori, io che sono l'ultimo devo svegliare l'arbitro in attesa e poi mi blocco attendendo il via.
        corsa->ultimo = numerocorridore;
        sem_post(&corsa->arbitro);
    }
    else if (corsa->corr_arrivati == 1)
    {
        printf("Corridore %d> Sono il primo, attendo la fine dell'arbitro (io il mio l'ho fatto)\n", numerocorridore);
        corsa->primo = numerocorridore;
    }
    sem_post(&corsa->corsetta);
}
void arbitro_attendicorridori(struct corsa_t *corsa)
{
    printf("Aspetto i corridori al via\n");

    sem_wait(&corsa->arbitro);
    printf("Tutti i corridori sono al via\n");
}

void arbitro_via(struct corsa_t *corsa)
{
    // tutti sono al via
    sem_wait(&corsa->corsetta);

    // sveglio tutti i corridori
    printf("********  VIA *****************\n");
    for (int i = 0; i < NCORRIDORI; i++)
    {
        sem_post(&corsa->corridori);
    }

    sem_post(&corsa->corsetta);
}
void arbitro_risultato(struct corsa_t *corsa, int *primo, int *ultimo)
{
    sem_wait(&corsa->corsetta);

    if (corsa->corr_arrivati != NCORRIDORI)
    {
        // non sono arrivati tutti, mi blocco
        sem_post(&corsa->corsetta);
        sem_wait(&corsa->arbitro);
    }

    *primo = corsa->primo;
    *ultimo = corsa->ultimo;

    corsa->primo = -1;
    corsa->ultimo = -1;
    corsa->corr_al_via = 0;
    corsa->corr_arrivati = 0;

    sem_post(&corsa->corsetta);
    printf("Primo della gara --> Giocatore %d!\nUltimo della gara --> Giocatore %d\n", first, last);
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
    printf("****** FINE GARA ******\n");

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