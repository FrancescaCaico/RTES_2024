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
/*Una discoteca ha la seguente organizzazione per far entrare i clienti nella sala da ballo esiste una cassiera che eminent VOLTA. Emissione del biglietto e riscossione partono assieme, ma alla fine sono non bloccanti.

per evitare confusione alla cassa esiste una porta che fa attendere i clienti fuori dalla discoteca La porta all'inizio del sistema è aperta. All'arrivo del primo cliente la porta si chiude dietro di lui. Quando l'ULTIMO cliente in coda alla cassa ha ritirato il biglietto (ovvero alla fine di cliente_esco_coda), se ci sono dei clienti in attesa fuori, si apre la porta, si fanno entrare (a vostra scelta se farli entrare tutti o solo parte di essi), dopo di che la porta si richiude di nuovo. Se non ci sono clienti in attesa fuori la porta viene aperta.

Il sistema è modellato tramite un thread per la cassiera, ed uno per ogni cliente. Il numero di clienti non è specificato. Non è importante l'ordine di accodamento dei clienti.*/

struct discoteca_t
{

} discoteca;

void init_discoteca(struct discoteca_t *d)
{
}

void cliente_coda_fuori(struct discoteca_t *d)
{
}

void cliente_coda_dentro(struct discoteca_t *d)
{
}

void cliente_esco_coda(struct discoteca_t *d)
{
}

void cassiera_attesa_cliente(struct discoteca_t *d)
{
}

void cassiera_cliente_servito(struct discoteca_t *d)
{
}

void *cliente(void *arg)
{

    // ARRIVA ALL'INGRESSO DELLA DISCO
    cliente_coda_fuori(&discoteca);
    // VADO A FARE IL BIGLIETTO DALLA CASSIERA
    cliente_coda_dentro(&discoteca);
    // PAGO E RITIRO IL BIGLIETTO
    cliente_esco_coda(&discoteca);
    // A BALLARE
}

void *cassiera(void *arg)
{
    while (1)
    {
        cassiera_attesa_cliente(&discoteca);
        // EMISSIONE BIGLIETTO E RISCOSSIONE SINGOLO CLIENTE
        cassiera_cliente_servito(&discoteca);
        // METTO A POSTO I SOLDINI
    }
}
