/*
All’interno di un’applicazione, vari processi (ciclici) cooperano, secondo i criteri specificati nel seguito,
accedendo ad una risorsa condivisa R.
Alcuni fra questi processi, arrivati ad un certo punto della loro esecuzione, per procedere devono
controllare che si sia verificato un certo evento (che indicheremo come eventoA). In caso positivo
procedono senza bloccarsi altrimenti attendono che l’evento si verifichi. Questo controllo viene effettuato
accedendo alla risorsa R tramite la funzione testaA. Altri processi sono preposti a segnalare l’occorrenza
dell’evento invocando la funzione segnalaA. Gli eventi segnalati sono però, per così dire, “consumabili”.
In particolare, se un processo, invocando testaA verifica che eventoA si è già verificato, procede senza
bloccarsi ma resetta l’evento, cioè, da quel momento in poi se un altro processo invoca testaA, questo si
deve bloccare in attesa che, tramite segnalaA l’evento sia di nuovo segnalato. Se all’atto dell’invocazione
di testaA l’evento non si è ancora verificato, o se verificato è già stato consumato, allora il processo
richiedente si blocca. Viceversa, quando viene invocata segnalaA, se non ci sono processi in attesa
dell’evento, allora questa segnalazione resta disponibile per il primo processo che invochi testaA, se
viceversa uno o più processi sono in attesa dell’evento, tutti vengono riattivati e l’evento viene
consumato. Infine, se all’atto dell’invocazione di segnalaA e` ancora presente un precedente evento non
consumato, il processo segnalante si deve bloccare in attesa che il precedente evento venga consumato.
1- utilizzando il meccanismo semaforico implementare R con le due funzioni testaA e segnalaA.
2- Supponendo che i processi che segnalano l’evento siano P1, …., PN, e che ciascun processo
nell’invocare la funzione segnalaA passi il proprio nome (intero tra 1 e N) come parametro,
modificare la precedente soluzione imponendo una priorità tra questi processi quando devono
essere risvegliati dopo un blocco in modo tale che P1 abbia priorità su P2 ecc.


*/