#include <stdio.h>
#include <semaphore.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
int nanosleep(const struct timespec *req, struct timespec *rem);

/* Possibili stati in cui si puo' trovare la risorsa */
#define SEQUENZA_NESSUNO 0
#define SEQUENZA_A       1
#define SEQUENZA_B       2
#define SEQUENZA_C       3
#define SEQUENZA_D       4
#define SEQUENZA_E       5
#define SEQUENZA_D_o_E   6

#define _POSIX_C_SOURCE      199309L

/* NOTA BENE:
    
il testo in realtà non impone nessuna priorità tra (A e C) e (D ed
E). ciò vuol dire che in questo caso bastano 3 semafori privati:

- uno per A+C
- uno per B
- uno per D+E

in quanto mi interessa svegliare uno dei bloccati (sia esso A o C)
indipendentemente dal suo tipo.

Notare che, nonostante ci siano solo 3 semafori privati, continuano ad
essere necessari tutti e 5 gli stati.

L'esempio qui riportato propone una soluzione con 5 semafori privati/5
variabili condition, uno per ogni categoria di funzione. In questo
caso avere piu' semafori permette di dare priorità ad A rispetto a C,
e a D rispetto ad E.
*/


/* la struttura condivisa */
struct gestore_t {
  sem_t mutex;

  sem_t sa,sb,sc,sd,se;
  int ca,cb,cc,cd,ce;

  /* stato del sistema */
  int nb;  
  int stato;
} gestore;

/* inizializzazione della struttura condivisa */
void init_gestore(struct gestore_t *g)
{
  /* mutua esclusione */
  sem_init(&g->mutex,0,1);

  /* semafori e contatori privati */
  sem_init(&g->sa,0,0);
  sem_init(&g->sb,0,0);
  sem_init(&g->sc,0,0);
  sem_init(&g->sd,0,0);
  sem_init(&g->se,0,0);

  g->ca = g->cb = g->cc = g->cd = g->ce = 0;

  /* stato del sistema */
  g->nb = 0;
  g->stato = SEQUENZA_NESSUNO;
}

void StartA(struct gestore_t *g)
{
  sem_wait(&g->mutex);
  if (g->stato == SEQUENZA_NESSUNO) {
    /* non mi blocco! */
    g->stato = SEQUENZA_A;
    sem_post(&g->sa);
  }
  else {
    /* mi blocco */
    g->ca++;
  } 
  sem_post(&g->mutex);
  sem_wait(&g->sa);
}

void EndA(struct gestore_t *g)
{
  sem_wait(&g->mutex);

  g->stato = SEQUENZA_B;

  /* devo svegliare tutti i B pendenti */
  while (g->cb) {
    g->cb--;
    g->nb++;
    sem_post(&g->sb);
  }

  sem_post(&g->mutex);
}

void StartB(struct gestore_t *g)
{
  sem_wait(&g->mutex);

  if (g->stato == SEQUENZA_B) {
    /* devo contare i B perchè ce ne possono essere + di 1 */
    g->nb++;
    sem_post(&g->sb);
  }
  else {
    g->cb++;
  }

  sem_post(&g->mutex);
  sem_wait(&g->sb);
}

void sveglia_A_o_C(struct gestore_t *g)
{
  /* chiamata alla fine di B, D o E per controllare se devo svegliare
     qualcuno alla fine di una sequenza */
  if (g->ca) {
    g->ca--;
    g->stato = SEQUENZA_A;
    sem_post(&g->sa);
  } 
  else if (g->cc) {
    g->cc--;
    g->stato = SEQUENZA_C;
    sem_post(&g->sc);
  }
  else
    g->stato = SEQUENZA_NESSUNO;
}

void EndB(struct gestore_t *g)
{
  sem_wait(&g->mutex);

  g->nb--;

  if (g->nb == 0)
    sveglia_A_o_C(g);

  sem_post(&g->mutex);
}

void StartC(struct gestore_t *g)
{
  sem_wait(&g->mutex);
  if (g->stato == SEQUENZA_NESSUNO) {
    g->stato = SEQUENZA_C;
    sem_post(&g->sc);
  }
  else {
    g->cc++;
  } 
  sem_post(&g->mutex);
  sem_wait(&g->sc);
}

void EndC(struct gestore_t *g)
{
  sem_wait(&g->mutex);

  if (g->cd) {
    g->cd--;
    g->stato = SEQUENZA_D;
    sem_post(&g->sd);
  } 
  else if (g->ce) {
    g->ce--;
    g->stato = SEQUENZA_E;
    sem_post(&g->se);
  }
  else
    /* se non c'e' nessuno bloccato, so comunque che potra' entrare
       solo un D o un E */
    g->stato = SEQUENZA_D_o_E;

  sem_post(&g->mutex);
}

void StartD(struct gestore_t *g)
{
  sem_wait(&g->mutex);
  if (g->stato == SEQUENZA_D_o_E) {
    g->stato = SEQUENZA_D;
    sem_post(&g->sd);
  }
  else {
    g->cd++;
  } 
  sem_post(&g->mutex);
  sem_wait(&g->sd);
}

void EndD(struct gestore_t *g)
{
  sem_wait(&g->mutex);

  sveglia_A_o_C(g);

  sem_post(&g->mutex);
}

void StartE(struct gestore_t *g)
{
  sem_wait(&g->mutex);
  if (g->stato == SEQUENZA_D_o_E) {
    g->stato = SEQUENZA_E;
    sem_post(&g->se);
  }
  else {
    g->ce++;
  } 
  sem_post(&g->mutex);
  sem_wait(&g->se);
}

void EndE(struct gestore_t *g)
{
  sem_wait(&g->mutex);

  sveglia_A_o_C(g);

  sem_post(&g->mutex);
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
  for (;;) {
    StartA(&gestore);
    putchar(*(char *)arg);
    EndA(&gestore);
    pausetta();
  }
  return 0;
}

void *B(void *arg)
{
  for (;;) {
    StartB(&gestore);
    putchar(*(char *)arg);
    EndB(&gestore);
    pausetta();
  }
  return 0;
}

void *C(void *arg)
{
  for (;;) {
    StartC(&gestore);
    putchar(*(char *)arg);
    EndC(&gestore);
    pausetta();
  }
  return 0;
}

void *D(void *arg)
{
  for (;;) {
    StartD(&gestore);
    putchar(*(char *)arg);
    EndD(&gestore);
    pausetta();
  }
  return 0;
}

void *E(void *arg)
{
  for (;;) {
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


