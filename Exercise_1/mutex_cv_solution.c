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
#include <time.h>
int nanosleep(const struct timespec *req, struct timespec *rem);

#define STATO_NESSUNO 0
#define STATO_A 1
#define STATO_B 2
#define STATO_C 3
#define STATO_E 4
#define STATO_D 5
#define STATO_D_E 6

struct gestore_t
{
  // mutex + condition variable.
  pthread_mutex_t mutex; // per proteggere le sez critiche. --> mutia esclusione

  // CONDITION VARIABLES --> per la sync delle varie classi di processi
  pthread_cond_t sa, sb, sc, sd, se;
  // CONTATORI DI QUANTI SONO IN CODA SULLA CONDITION VARIABLE
  int blocked_a, blocked_b, blocked_c, blocked_d, blocked_e;

  // CONTATORE PER SAPERE QUANTI B SONO ATTIVI --> NECESSARIO PER CONOSCERE L'ULTIMO
  int active_b;
  int status;

} gestore;

void init_gestore(struct gestore_t *g)
{

  pthread_mutexattr_t mutex_attr;
  pthread_condattr_t cond_attr;

  pthread_mutexattr_init(&mutex_attr);
  pthread_condattr_init(&cond_attr);
  pthread_mutex_init(&g->mutex, &mutex_attr); // mutua esclusione

  pthread_cond_init(&g->sa, &cond_attr);
  pthread_cond_init(&g->sb, &cond_attr);
  pthread_cond_init(&g->sc, &cond_attr);
  pthread_cond_init(&g->sd, &cond_attr);
  pthread_cond_init(&g->se, &cond_attr);

  pthread_mutexattr_destroy(&mutex_attr);
  pthread_condattr_init(&cond_attr);

  g->blocked_a = g->blocked_b = g->blocked_c = g->blocked_d = g->blocked_e = 0;

  g->active_b = 0;
  g->status = STATO_NESSUNO;
}

void WakeUpAorC(struct gestore_t *g)
{

  if (g->blocked_a)
  {
    pthread_cond_signal(&g->sa);
  }
  else if (g->blocked_c)
  {
    pthread_cond_signal(&g->sc);
  }

  g->status = STATO_NESSUNO;
}

void WakeUpDorE(struct gestore_t *g)
{

  if (g->blocked_d)
  {
    pthread_cond_signal(&g->sd);
  }
  else if (g->blocked_e)
  {
    pthread_cond_signal(&g->se);
  }

  g->status = STATO_D_E;
}

void StartA(struct gestore_t *g)
{
  // prendo il mutex per accedere alla struttura condivisa
  pthread_mutex_lock(&g->mutex);

  // se lo stato è nessuno posso accedere.
  while (g->status != STATO_NESSUNO)
  {
    // NON POSSO ACCEDERE
    g->blocked_a++;
    // MI BLOCCO SULLA VARIABILE CONDIZIONE
    pthread_cond_wait(&g->sa, &g->mutex); // il mutex viene automaticamente rilasciato e ripreso se qualcuno mi sveglia
    // Se qualcuno mi risveglia, parto da qui --> mi attivo e rieseguo la condizione del while per vedere che nessuno abbia preso già il mio posto.
    g->blocked_a--;
  }

  // arrivati qui sono entrato come thread. Posso quindi cambiare lo stato e rilasciare il mutex.
  g->status = STATO_A;
  pthread_mutex_unlock(&g->mutex);
}

void EndA(struct gestore_t *g)
{

  // prendo il mutex per accedere alla struttura condivisa
  pthread_mutex_lock(&g->mutex);

  // DOMANDA DA PORSI... CHI SVEGLIARE? B. se non ci sono B si aspettano perchè esclusivamente da A si deve passare a B
  g->status = STATO_B;
  if (g->blocked_b)
  {
    pthread_cond_broadcast(&g->sb);
  }

  pthread_mutex_unlock(&g->mutex);
}

void StartB(struct gestore_t *g)
{
  // prendo il mutex per accedere alla struttura condivisa
  pthread_mutex_lock(&g->mutex);

  while (g->status != STATO_B)
  {
    g->blocked_b++;
    pthread_cond_wait(&g->sb, &g->mutex);
    g->blocked_b--;
  }
  g->active_b++;
  // g->status = STATO_B;
  pthread_mutex_unlock(&g->mutex);
}

void EndB(struct gestore_t *g)
{
  pthread_mutex_lock(&g->mutex);
  g->active_b--;
  if (g->active_b == 0)
  {
    // devo svegliare un A o C
    WakeUpAorC(g);
  }

  pthread_mutex_unlock(&g->mutex);
}

void StartC(struct gestore_t *g)
{

  pthread_mutex_lock(&g->mutex);

  while (g->status != STATO_NESSUNO)
  {
    g->blocked_c++;
    pthread_cond_wait(&g->sc, &g->mutex);
    g->blocked_c--;
  }
  g->status = STATO_C;

  pthread_mutex_unlock(&g->mutex);
}

void EndC(struct gestore_t *g)
{

  pthread_mutex_lock(&g->mutex);

  // devo svegliare un A o C
  WakeUpDorE(g);

  pthread_mutex_unlock(&g->mutex);
}

void StartD(struct gestore_t *g)
{
  pthread_mutex_lock(&g->mutex);

  while (g->status != STATO_D_E)
  {
    g->blocked_d++;
    pthread_cond_wait(&g->sd, &g->mutex);
    g->blocked_d--;
  }
  g->status = STATO_D;

  pthread_mutex_unlock(&g->mutex);
}

void EndD(struct gestore_t *g)
{
  pthread_mutex_lock(&g->mutex);

  WakeUpAorC(g);

  pthread_mutex_unlock(&g->mutex);
}

void StartE(struct gestore_t *g)
{
  pthread_mutex_lock(&g->mutex);

  while (g->status != STATO_D_E)
  {
    g->blocked_e++;
    pthread_cond_wait(&g->se, &g->mutex);
    g->blocked_e--;
  }
  g->status = STATO_E;

  pthread_mutex_unlock(&g->mutex);
}

void EndE(struct gestore_t *g)
{
  EndD(g);
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
    StartA(&gestore);
    putchar(*(char *)arg);
    EndA(&gestore);
    pausetta();
  }
  return 0;
}

void *B(void *arg)
{
  for (;;)
  {
    StartB(&gestore);
    putchar(*(char *)arg);
    EndB(&gestore);
    pausetta();
  }
  return 0;
}

void *C(void *arg)
{
  for (;;)
  {
    StartC(&gestore);
    putchar(*(char *)arg);
    EndC(&gestore);
    pausetta();
  }
  return 0;
}

void *D(void *arg)
{
  for (;;)
  {
    StartD(&gestore);
    putchar(*(char *)arg);
    EndD(&gestore);
    pausetta();
  }
  return 0;
}

void *E(void *arg)
{
  for (;;)
  {
    StartE(&gestore);
    putchar(*(char *)arg);
    EndE(&gestore);
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
  init_gestore(&gestore);

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
