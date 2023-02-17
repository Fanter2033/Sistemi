# DESCRIZIONE PROGETTO
Lo scopo della fase 1 del progetto è l'implementazione del livello 2 del S.O. Panda+. 
In particolare:
- allocazione e Deallocazione dei PCB
- gestione delle code dei PCB
- gestione degli alberi dei PCB
- gestione di una Active Semaphore Hash (ASH)
- gestione dei namespace

# SCELTE PROGETTUALI

## pcb.h
### Queue
Unica nota dell'implementazione riguarda outProcQ. Si è preferito fare il return della funzione nel ciclo per ottimizzare la memoria utilizzata.
### Tree
Nell'implementazione degli alberi utilizziamo solamente il campo next del campo p_child del PCB. Utilizzare il campo prev del p_child risulterebbe ridondante in quanto già presente il campo p_parent nel PCB

## ash.h
La funzione "removeEmptySemd" viene chiamata in removeBlocked e outBlocked. Si occupa di eseguire la parte comune alle due funzioni, ovvero la rimozione del semaforo se risultasse vuoto. 
Nelle chiamate alle funzioni di "hashtable.h" utilizziamo un cast a u32 del semadd, poichè le funzioni hash richiedono in input un unsigned 32.

## ns.h
Nell'inizializzazione dei namespace utilizziamo una matrice, nonostante per completare la fase 1 basterebbe un vettore. Questo per agevolare e anticipare lo sviluppo della fase 2.
Inoltre il tipo dei namespace viene già inizializzato in initNamespaces(), dunque durante l'allocazione non verranno modificati, ma verranno semplicemente scelti dalla lista corretta. 

# COMPILAZIONE
