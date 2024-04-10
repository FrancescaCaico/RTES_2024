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
