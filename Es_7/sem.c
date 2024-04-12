/*Esercizio 7
Un negozio di barbieri ha tre barbieri, e tre poltrone su cui siedono i clienti quando vengono per tagliarsi
la barba.
C'è una sala d'aspetto con un divano (max 4 persone; gli altri aspettano fuori dalla porta).
C'e' un cassiere, che può servire solamente un cliente (con barba tagliata) alla volta.
Scrivere il processo cliente che cerca di entrare nel negozio per farsi tagliare la barba,
Suggerimenti:
– considerare i barbieri, il cassiere ed il divano come risorse condivise.
– modellare il processo di taglio barba come un ciclo di SHAVING_ITERATIONS iterazioni
– modellare il processo di pagamento come un ciclo di PAYING_ITERATIONS iterazioni
– dopo che un cliente ha pagato esce (th thread muore) oppure, dopo un delay di alcuni secondi (usare la
primitiva sleep() ), si accoda nuovamente per farsi tagliare la barba.*/

#include <stdio.h>
#include <semaphore.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <time.h>
#include <stdbool.h>
#include <stdint.h>

#define NUM_BARBIERI 3
#define NUM_POLTRONE
#define CASSIERE 1
#define NUM_CLIENTI 10 // Numero di clienti che entrano nel negozio

#define SHAVING_ITERATIONS 3
#define PAYING_ITERATIONS 3

int nanosleep(const struct timespec *req, struct timespec *rem);

void pausetta(void)
{
    struct timespec t;
    t.tv_sec = 0;
    t.tv_nsec = (rand() % 10 + 1) * 1000000;
    nanosleep(&t, NULL);
}

struct barbershop_t
{
    sem_t mutex;          // per l'accesso alle variabili condivise
    sem_t divano;         // per attendere l'accesso al negozio aspetto che un posto sul divano si liberi
    sem_t attesa_taglio;  // aspetto che un barbiere si liberi
    sem_t cassiere;       // aspetto che il cassiere sia disponibile per il pagamento
    int b_liberi,         // quanti barbieri sono disponibili = al numero di poltrone vuote.
        c_disponibile,    // bisogna sapere se il cassiere è disponibile per il pagamento
        num_posti_divano, // quanti posti sono disponibili sul divano per accedere eventualmente al negozio.
        attesa_cassiere,  // quanti abbiamo finito il taglio ma il cassiere è occupato (siamo in fila per pagare in attesa_cassiere)
        attesa_divano;    // quanti sono in attesa del divano ovvero di accedere
} barbershop;

void initBarberShop(struct barbershop_t *barbershop)
{
    sem_init(&barbershop->mutex, 0, 1);
    sem_init(&barbershop->cassiere, 0, 0);
    sem_init(&barbershop->divano, 0, 0);
    sem_init(&barbershop->attesa_taglio, 0, 0);

    barbershop->b_liberi = 3; // uguale al numero di poltrone libere per il taglio
    barbershop->c_disponibile = 1;
    barbershop->attesa_cassiere = 0;
    barbershop->num_posti_divano = 4;
    barbershop->attesa_divano = 0;
}

void taglioBarba()
{
    for (int i = 0; i < SHAVING_ITERATIONS; ++i)
    {
        printf("%ld > Mi stanno tagliando la barba\n", pthread_self());
    }
}

void pagamentoBarba()
{
    for (int i = 0; i < SHAVING_ITERATIONS; ++i)
    {
        printf("%ld > Sto pagando\n", pthread_self());
    }
}

void entraBarberShop(struct barbershop_t *b)
{
    sem_wait(&b->mutex);
    printf("%ld> Attualmente i posti sul divano --> %d e ci sono %d barbieri liberi\n", pthread_self(), b->num_posti_divano, b->b_liberi);
    if (b->num_posti_divano > 0)
    {

        // c'è un posto sul divano. Posso sedermi

        b->num_posti_divano--;
        printf("%ld > Ho preso un posto ne restano %d\n", pthread_self(), b->num_posti_divano);
        sem_post(&b->divano);
    }
    else
    {
        // non ci sono posti liberi, devo bloccarmi in attesa che un posto sia libero
        b->attesa_divano++;
        printf("%ld > Mi blocco in attesa di un posto sul divano\n", pthread_self());
    }
    sem_post(&b->mutex);
    sem_wait(&b->divano); // aspetta fuori in attesa che qualcuno esca.
    printf("%ld> Mi sono seduto, aspetto un barbiere libero eventualmente\n", pthread_self());
}

void attendiTurno(struct barbershop_t *b)
{
    // sono seduto sul divano
    sem_wait(&b->mutex);
    if (b->b_liberi)
    {
        // posso tagliarmi i capelli.
        //  posso entrare nel barbershop
        b->b_liberi--;
        sem_post(&b->attesa_taglio);
    }
    sem_post(&b->mutex);
    sem_wait(&b->attesa_taglio);
}

void sieditiPoltrona(struct barbershop_t *b)
{
    sem_wait(&b->mutex);
    // chi sblocco?
    printf("%ld > C'è un barbiere libero. Mi siedo per il taglio, restano %d barbieri liberi\n", pthread_self(), b->b_liberi);

    b->num_posti_divano++;
    // chi è seduto sul divano, se c'è qualcuno fuori
    if (b->attesa_divano)
    {
        b->attesa_divano--;
        b->num_posti_divano--;
        sem_post(&b->divano);
    }
    sem_post(&b->mutex);
};

void pagaServizio(struct barbershop_t *b)
{
    sem_wait(&b->mutex);
    b->b_liberi++;
    // il cassiere non è un thread.
    // devo svegliare chi aspetta sul divano.
    if (b->num_posti_divano < 4)
    {
        // c'è almeno qualcuno seduto
        b->b_liberi--;
        sem_post(&b->attesa_taglio);
    }
    if (b->c_disponibile)
    {
        // posso pagare
        b->c_disponibile = 0;
        sem_post(&b->cassiere);
    }
    else
    {
        b->attesa_cassiere++;
    }
    sem_post(&b->mutex);
    sem_wait(&b->cassiere);
}

void escoBarberShop(struct barbershop_t *b)
{
    sem_wait(&b->mutex);

    b->c_disponibile = 1;
    if (b->attesa_cassiere)
    {
        b->c_disponibile = 0;
        b->attesa_cassiere--;
        sem_post(&b->cassiere);
    }

    sem_post(&b->mutex);
}

void *cliente(void *arg)
{
    int cliente_id = (intptr_t)arg;

    pausetta();
    printf("Cliente %ld (%d): Cerco di entrare nel negozio.\n", pthread_self(), cliente_id);
    entraBarberShop(&barbershop);

    attendiTurno(&barbershop);

    sieditiPoltrona(&barbershop);

    taglioBarba();
    pagaServizio(&barbershop);
    pagamentoBarba();
    pausetta();
    printf("Cliente %ld (%d): Ho finito di tagliarmi la barba e pagare. Esco.\n", pthread_self(), cliente_id);
    escoBarberShop(&barbershop);
    return NULL;
}

int main()
{
    pthread_attr_t a;
    pthread_t p;

    /* inizializzo il mio sistema */
    initBarberShop(&barbershop);

    /* inizializzo i numeri casuali, usati nella funzione pausetta */
    srand(555);

    pthread_attr_init(&a);

    /* non ho voglia di scrivere 10000 volte join! */
    pthread_attr_setdetachstate(&a, PTHREAD_CREATE_DETACHED);

    intptr_t i = 0;
    while (i < NUM_CLIENTI)
    {
        pthread_create(&p, &a, cliente, (void *)i);
        i++;
    }

    pthread_attr_destroy(&a);
    /* aspetto 10 secondi prima di terminare tutti quanti */
    sleep(10);
    printf("%s", "Chiusura!\n");

    return 0;
}