/*
Esercizio 10
In un sistema organizzato secondo il modello a memoria comune due risorse RA e RB sono allocate
dinamicamente ai processi P1, P2, …., Pn tramite un comune gestore G. Ogni processo può richiedere al
gestore l’uso esclusivo della risorsa RA (tramite la funzione RicA) o della risorsa RB (tramite la funzione
RicB) oppure l’uso esclusivo di una qualunque delle due (tramite la funzione RicQ). Un processo può
però richiedere anche l’uso esclusivo di entrambe le risorse (tramite la funzione Ric2). Chiaramente,
dopo che la risorsa (o le risorse) è stata (sono state) utilizzata (utilizzate), il processo la (le) rilascia al
gestore tramite le opportune funzioni di rilascio.
Utilizzando il meccanismo semaforico, realizzare il gestore G con tutte le funzioni di richiesta e di
rilascio necessarie per implementare quanto sopra specificato. Nel realizzare il gestore si tenga conto
delle seguenti regole di priorità: vengono privilegiate le richieste generiche rispetto alle richieste della
risorsa RA e queste nei confronti delle richieste della risorsa RB e infine queste nei confronti delle
richieste di entrambe le risorse. Durante l’allocazione di una risorsa, in seguito ad una richiesta generica,
se sono disponibili entrambe le risorse, allocare per prima la risorsa RA.*/