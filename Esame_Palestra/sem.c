#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdint.h>

#define N 3  // tipologie di attrezzi
#define M 2  // unità per tipologia di attrezzi
#define P 10 // persone nella palestra
#define E 4  // esercizi che vogliono svoglere le persone

struct palestra_t
{

    sem_t mutex;
    sem_t attrezzo[N];
    int bloccati_attr[N];
    int prenotazioni[P]; // si mette l'indice dell'attrezzo da usare, quando si prenota si decrementa automaticamente cosicchè nessun altro possa prendere l'attrezzo prenotato.
    int unita_libere_a[N];

} palestra;

void init_palestra(struct palestra_t *s)
{
    sem_init(&s->mutex, 0, 1);

    for (int i = 0; i < N; ++i)
    {
        sem_init(&s->attrezzo[i], 0, 0);
        s->unita_libere_a[i] = M;
        s->bloccati_attr[i] = -1;
    }

    for (int i = 0; i < P; ++i)
    {
        s->prenotazioni[i] = -1;
    }
}

void usaattrezzo(struct palestra_t *p, int numeropersona, int tipoattrezzo)
{

    sem_wait(&p->mutex);
    printf("Persona %d: Cerco di usare attrezzo di tipo %d\n", numeropersona, tipoattrezzo);
    // devo eseguire il while solo se non ho prenotato l'attrezzo altrimenti lo posso usare senza bloccarmi.
    if (p->prenotazioni[numeropersona] != tipoattrezzo)
    {
        // mdevo eseguire il while
        if (p->unita_libere_a[tipoattrezzo])
        {
            printf("Persona %d: Ottengo l'attrezzo di tipo %d\n", numeropersona, tipoattrezzo);
            p->unita_libere_a[tipoattrezzo]--;
            sem_post(&p->attrezzo[tipoattrezzo]);
        }
        else
        {
            printf("Persona %d: Attendo l'attrezzo di tipo %d\n", numeropersona, tipoattrezzo);
            p->bloccati_attr[tipoattrezzo]++;
        }
    }
    else
    {
        printf("Persona %d: Avevo già prenotato l'attrezzo di tipo %d\n", numeropersona, tipoattrezzo);
        sem_post(&p->attrezzo[tipoattrezzo]);
    }

    sem_post(&p->mutex);
    sem_wait(&p->attrezzo[tipoattrezzo]);
    printf("Persona %d: Uso l'attrezzo di tipo %d\n", numeropersona, tipoattrezzo);
}

void fineuso(struct palestra_t *p, int numeropersona, int tipoattrezzo)
{

    sem_wait(&p->mutex);
    printf("Persona %d: Restituisco l'attrezzo di tipo %d\n", numeropersona, tipoattrezzo);
    p->unita_libere_a[tipoattrezzo]++;
    if (p->bloccati_attr[tipoattrezzo] && p->unita_libere_a[tipoattrezzo] != 0)
    {
        // li sveglio
        p->unita_libere_a[tipoattrezzo]--;
        p->bloccati_attr[tipoattrezzo]--;
        sem_post(&p->attrezzo[tipoattrezzo]);
    }
    sem_post(&p->mutex);
}

void prenota(struct palestra_t *p, int numeropersona, int tipoattrezzo)
{

    sem_wait(&p->mutex);

    if (p->unita_libere_a[tipoattrezzo])
    {
        printf("Persona %d: Prenoto attrezzo di tipo %d\n", numeropersona, tipoattrezzo);
        p->prenotazioni[numeropersona] = tipoattrezzo;
        p->unita_libere_a[tipoattrezzo]--;
    }
    else
    {
        printf("Persona %d: Non posso prenotare attrezzo di tipo %d, nessuna unità libera\n", numeropersona, tipoattrezzo);
        p->prenotazioni[numeropersona] = -1;
    }
    // se non posso prenotare resta a -1;
    sem_post(&p->mutex);
}

void *persona(void *arg)
{

    int pers = (intptr_t)arg;
    int attrezzo_corrente = rand() % N;
    int prossimoattrezzo = rand() % N;
    for (int i = E; i > 0; i--)
    {
        usaattrezzo(&palestra, pers, attrezzo_corrente);
        if (i != 0)
            prenota(&palestra, pers, attrezzo_corrente);
        fineuso(&palestra, pers, attrezzo_corrente);
        if (i != 0)
        {
            attrezzo_corrente = prossimoattrezzo;
            prossimoattrezzo = rand() % N;
        }
    }
    return 0;
}

int main()
{
    pthread_attr_t a;
    pthread_t persone[P];

    init_palestra(&palestra);

    srand(time(NULL));

    pthread_attr_init(&a);
    pthread_attr_setdetachstate(&a, PTHREAD_CREATE_DETACHED);

    for (intptr_t i = 0; i < P; i++)
    {
        pthread_create(&persone[i], &a, persona, (void *)i);
    }

    pthread_attr_destroy(&a);

    sleep(15);

    return 0;
}