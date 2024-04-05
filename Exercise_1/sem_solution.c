/*Esercizio 1
Siano A, B, C, D ed E le procedure che un insieme di processi P1, P2, ..., PN possono invocare e che
devono essere eseguite rispettando i seguenti vincoli di sincronizzazione:
Sono possibili solo due sequenze di esecuzioni delle procedure, sequenze tra loro mutuamente esclusive:
    - la prima sequenza prevede che venga eseguita per prima la procedura A.
      a cui puo' seguire esclusivamente líesecuzione di una o piu' attivazioni concorrenti della procedura B;
    - la seconda sequenza e' costituita dallíesecuzione della procedura C
      a cui puo' seguire esclusivamente l'esecuzione della procedura D, o in alternativa a D della procedura E.
Una volta terminata una delle due sequenze una nuova sequenza puo' essere di nuovo iniziata.
utilizzando il meccanismo dei semafori, realizzare le funzioni StartA, EndA, StartB, EndB, .... , StartE,
EndE che, invocate dai processi clienti P1, P2, ..., PN rispettivamente prima e dopo le corrispondenti
procedure, garantiscano il rispetto dei precedenti vincoli. Nel risolvere il problema non e' richiesta la
soluzione ad eventuali problemi di starvation.
*/

#include <stdio.h>
#include <semaphore.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>

// si ragiona a stati

#define NESSUNO 1
#define STATO_A 2
#define STATO_B 3
#define STATO_C 4
#define STATO_E_D 5
#define STATO_E 6
#define STATO_D 7

// Non c'e' priorita' tra la sequenza A o C
// Non c'e' priorita' tra E o D
// Una sola sequenza per volta deve eseguire

struct gestore_t
{
  sem_t mutex_seq; // per proteggere le sezioni critiche

  sem_t sa, sb, sc, sd, se; // per accedere ad una procedura  --> DICE ANCHE QUANTI BLOCCATI
  int ca, cb, cc, cd, ce;   // mantengo il numero di task bloccati

  int nb;    // dato che più b possono accedere dobbiamo tenerne conto di quanti sono in esecuzione.
  int stato; // controlliamo lo stato a seconda dei vari interleaving e di chi prende cosa.
} gestore;

void inizializzaGestore(struct gestore_t *g)
{

  // inizializzo il primo mutex che è di mutua esclusione:
  sem_init(&g->mutex_seq, 0, 1);

  // inizializzo i mutex di sync e i relativi contatori

  sem_init(&g->sa, 0, 0);
  g->ca = 0;
  sem_init(&g->sb, 0, 0);
  g->cb = 0;
  sem_init(&g->sc, 0, 0);
  g->cc = 0;
  sem_init(&g->sd, 0, 0);
  g->cd = 0;
  sem_init(&g->se, 0, 0);
  g->cd = 0;

  g->nb = 0;
  g->stato = NESSUNO;
}

void sveglioAorC(struct gestore_t *g)
{

  if (g->ca)
  {
    // sveglio un A
    g->ca--;
    g->stato = STATO_A;
    sem_post(&g->sa);
  }
  else if (g->cc)
  {
    // sveglio un C
    g->cc--;
    g->stato = STATO_C;
    sem_post(&g->sa);
  }
  else
  {
    // non sveglio nessuno e cambio lo stato
    g->stato = NESSUNO;
  }
}

// inzio le procedure di prologo ed epilogo partendo dalla prima sequenza.

// prologo --> QUANDO MI BLOCCO?

void startA(struct gestore_t *g)
{

  // prima di consultare le variabili devo acquisire il mutex per essere sicuro di essere il solo a guardare, cosi nessuno può modificarle mentre io le consulto

  sem_wait(&g->mutex_seq);

  // uso la soluzione con il post-previa
  if (g->stato == NESSUNO)
  {
    g->stato = STATO_B;

    // eseguo preventivamente la post
    sem_post(&g->sa);
  }
  else
  {
    g->ca++;
  }
  sem_post(&g->mutex_seq);
  sem_wait(&g->sa);
}

// CHI DEVO SVEGLIARE?
void endA(struct gestore_t *g)
{

  // prima di consultare le variabili devo acquisire il mutex per essere sicuro di essere il solo a guardare, cosi nessuno può modificarle mentre io le consulto

  sem_wait(&g->mutex_seq);

  // prima di bloccarmi guardo se ci sono B in attesa e in tal caso dato che possono andare tutti insieme li sveglio tutti
  g->stato = STATO_B;
  while (g->cb != 0)
  {
    sem_post(&g->sb);
    g->cb--;
  }

  sem_post(&g->mutex_seq);
}

void startB(struct gestore_t *g)
{

  // prima di consultare le variabili devo acquisire il mutex per essere sicuro di essere il solo a guardare, cosi nessuno può modificarle mentre io le consulto

  sem_wait(&g->mutex_seq);

  // uso la soluzione con il post-previa
  if (g->stato == STATO_B)
  {
    g->nb++;
    // eseguo preventivamente la post
    sem_post(&g->sb);
  }
  else
  {
    g->cb++;
  }
  sem_post(&g->mutex_seq);
  sem_wait(&g->sb);
}

// CHI DEVO SVEGLIARE?
void endB(struct gestore_t *g)
{

  // prima di consultare le variabili devo acquisire il mutex per essere sicuro di essere il solo a guardare, cosi nessuno può modificarle mentre io le consulto

  sem_wait(&g->mutex_seq);

  // se sono l'ultimo B devo svegliare o un altro A o un C
  g->nb--;
  if (g->nb == 0)
  {
    // devo svegliare o A o C
    sveglioAorC(g);
  }

  sem_post(&g->mutex_seq);
}

void startC(struct gestore_t *g)
{

  // prima di consultare le variabili devo acquisire il mutex per essere sicuro di essere il solo a guardare, cosi nessuno può modificarle mentre io le consulto

  sem_wait(&g->mutex_seq);

  // uso la soluzione con il post-previa
  if (g->stato == NESSUNO)
  {
    g->stato = STATO_C;

    // eseguo preventivamente la post
    sem_post(&g->sc);
  }
  else
  {
    g->cc++;
  }
  sem_post(&g->mutex_seq);
  sem_wait(&g->sc);
}

void endC(struct gestore_t *g)
{

  // prima di consultare le variabili devo acquisire il mutex per essere sicuro di essere il solo a guardare, cosi nessuno può modificarle mentre io le consulto

  sem_wait(&g->mutex_seq);

  // devo svegliare o E o D
  if (g->cd)
  {
    /*sveglio un D e migro nello stato D*/
    g->cd--;
    g->stato = STATO_D;
    sem_post(&g->sd);
  }
  else if (g->ce)
  {
    /*sveglio un E e migro nello stato E*/
    g->ce--;
    g->stato = STATO_E;
    sem_post(&g->se);
  }
  else
  {
    /* migro nel mio stato E _ D dato che esclusivamente uno dei due deve partire. */
    g->stato = STATO_E_D;
  }

  sem_post(&g->mutex_seq);
}

void startD(struct gestore_t *g)
{

  // prima di consultare le variabili devo acquisire il mutex per essere sicuro di essere il solo a guardare, cosi nessuno può modificarle mentre io le consulto

  sem_wait(&g->mutex_seq);

  // uso la soluzione con il post-previa
  if (g->stato == STATO_E_D)
  {
    g->stato = STATO_D;
    // eseguo preventivamente la post
    sem_post(&g->sd);
  }
  else
  {
    g->cd++;
  }
  sem_post(&g->mutex_seq);
  sem_wait(&g->sd);
}

void endD(struct gestore_t *g)
{

  // prima di consultare le variabili devo acquisire il mutex per essere sicuro di essere il solo a guardare, cosi nessuno può modificarle mentre io le consulto

  sem_wait(&g->mutex_seq);

  sveglioAorC(g);

  sem_post(&g->mutex_seq);
}

void startE(struct gestore_t *g)
{

  // prima di consultare le variabili devo acquisire il mutex per essere sicuro di essere il solo a guardare, cosi nessuno può modificarle mentre io le consulto

  sem_wait(&g->mutex_seq);

  // uso la soluzione con il post-previa
  if (g->stato == STATO_E_D)
  {
    g->stato = STATO_E;
    // eseguo preventivamente la post
    sem_post(&g->se);
  }
  else
  {
    g->ce++;
  }
  sem_post(&g->mutex_seq);
  sem_wait(&g->se);
}

void endE(struct gestore_t *g)
{

  // prima di consultare le variabili devo acquisire il mutex per essere sicuro di essere il solo a guardare, cosi nessuno può modificarle mentre io le consulto

  sem_wait(&g->mutex_seq);

  sveglioAorC(g);

  sem_post(&g->mutex_seq);
}

void pausetta(void)
{
  struct timespec t;
  t.tv_sec = 0;
  t.tv_nsec = (rand() % 10 + 1) * 1000000;
  nanosleep(&t, NULL);
}

void *A(void *arg)
{
  for (;;)
  {
    startA(&gestore);
    putchar(*(char *)arg);
    endA(&gestore);
    pausetta();
  }
  return 0;
}

void *B(void *arg)
{
  for (;;)
  {
    startB(&gestore);
    putchar(*(char *)arg);
    endB(&gestore);
    pausetta();
  }
  return 0;
}

void *C(void *arg)
{
  for (;;)
  {
    startC(&gestore);
    putchar(*(char *)arg);
    endC(&gestore);
    pausetta();
  }
  return 0;
}

void *D(void *arg)
{
  for (;;)
  {
    startD(&gestore);
    putchar(*(char *)arg);
    endD(&gestore);
    pausetta();
  }
  return 0;
}

void *E(void *arg)
{
  for (;;)
  {
    startE(&gestore);
    putchar(*(char *)arg);
    endE(&gestore);
    pausetta();
  }
  return 0;
}

/* la creazione dei thread */

int main()
{
  pthread_attr_t a;
  pthread_t p;

  /* inizializzo il mio sistema */
  inizializzaGestore(&gestore);

  /* inizializzo i numeri casuali, usati nella funzione pausetta */
  srand(555);

  pthread_attr_init(&a);

  /* non ho voglia di scrivere 10000 volte join! */
  pthread_attr_setdetachstate(&a, PTHREAD_CREATE_DETACHED);

  pthread_create(&p, &a, A, (void *)"a");
  pthread_create(&p, &a, A, (void *)"A");

  pthread_create(&p, &a, B, (void *)"B");
  pthread_create(&p, &a, B, (void *)"b");
  pthread_create(&p, &a, B, (void *)"x");

  pthread_create(&p, &a, C, (void *)"C");
  pthread_create(&p, &a, C, (void *)"c");

  pthread_create(&p, &a, D, (void *)"D");
  pthread_create(&p, &a, D, (void *)"d");

  pthread_create(&p, &a, E, (void *)"E");
  pthread_create(&p, &a, E, (void *)"e");

  pthread_attr_destroy(&a);

  /* aspetto 10 secondi prima di terminare tutti quanti */
  sleep(10);

  return 0;
}
