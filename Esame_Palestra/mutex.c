#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdint.h>

#define N 6  // tipologie di attrezzi
#define M 2  // unità per tipologia di attrezzi
#define P 10 // persone nella palestra
#define E 4  // esercizi che vogliono svoglere le persone

struct palestra_t
{

    pthread_mutex_t mutex;
    pthread_cond_t attrezzo[N];
    int bloccati_attr[N];
    int prenotazioni[P]; // si mette l'indice dell'attrezzo da usare, quando si prenota si decrementa automaticamente cosicchè nessun altro possa prendere l'attrezzo prenotato.
    int unita_libere_a[N];

} palestra;

void init_palestra(struct palestra_t *s)
{
    pthread_mutexattr_t m;
    pthread_condattr_t c;

    pthread_mutexattr_init(&m);
    pthread_condattr_init(&c);

    for (int i = 0; i < N; ++i)
    {
        pthread_cond_init(&s->attrezzo[i], &c);
        s->unita_libere_a[i] = M;
        s->bloccati_attr[i] = -1;
    }

    for (int i = 0; i < P; ++i)
    {
        s->prenotazioni[i] = -1;
    }

    pthread_mutexattr_destroy(&m);
    pthread_condattr_destroy(&c);
}

void usaattrezzo(struct palestra_t *p, int numeropersona, int tipoattrezzo)
{

    pthread_mutex_lock(&p->mutex);
    printf("Persona %d: Cerco di usare attrezzo di tipo %d\n", numeropersona, tipoattrezzo);
    // devo eseguire il while solo se non ho prenotato l'attrezzo altrimenti lo posso usare senza bloccarmi.
    if (p->prenotazioni[numeropersona] != tipoattrezzo)
    {
        // mdevo eseguire il while
        while (!p->unita_libere_a[tipoattrezzo])
        {
            printf("Persona %d: Attendo l'attrezzo di tipo %d\n", numeropersona, tipoattrezzo);
            p->bloccati_attr[tipoattrezzo]++;
            pthread_cond_wait(&p->attrezzo[tipoattrezzo], &p->mutex);
            p->bloccati_attr[tipoattrezzo]--;
        }
        printf("Persona %d: Ottengo l'attrezzo di tipo %d\n", numeropersona, tipoattrezzo);
        p->unita_libere_a[tipoattrezzo]--;
    }
    else
    {
        printf("Persona %d: Avevo già prenotato l'attrezzo di tipo %d\n", numeropersona, tipoattrezzo);
    }
    printf("Persona %d: Uso l'attrezzo di tipo %d\n", numeropersona, tipoattrezzo);

    pthread_mutex_unlock(&p->mutex);
}

void fineuso(struct palestra_t *p, int numeropersona, int tipoattrezzo)
{

    pthread_mutex_lock(&p->mutex);
    printf("Persona %d: Restituisco l'attrezzo di tipo %d\n", numeropersona, tipoattrezzo);
    p->unita_libere_a[tipoattrezzo]++;
    if (p->bloccati_attr[tipoattrezzo])
    {
        // li sveglio
        pthread_cond_signal(&p->attrezzo[tipoattrezzo]);
    }
    pthread_mutex_unlock(&p->mutex);
}

void prenota(struct palestra_t *p, int numeropersona, int tipoattrezzo)
{

    pthread_mutex_lock(&p->mutex);

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
    pthread_mutex_unlock(&p->mutex);
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