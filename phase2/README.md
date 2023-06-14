# Descrizione progetto
Lo scopo della fase 2 del progetto è la realizzazione del livello 3 del S.O. Panda+
Ovvero la parte di kernel del S.O., in particolare:
- Inizializzazione del sistema 
- Scheduling dei processi 
- Gestione delle eccezioni
    - Syscalls
    - Interrupts
    - Traps

# Divisione File

La divisione scelta per i file del progetto è:
- initial.c:
    - Inizializza le variabili globali del nucleo e le strutture dati
    - Crea il primo processo
    - Inizializza i registri del primo processo
    - Invoca lo scheduler

- scheduler.c:  
    Componente principale che si occupa dello scheduling dei processi

- exceptions.c:  
    Si occupa di distinguere il tipo di eccezione, e di risolverla eventualmente affidandola al gestore opportuno:
    - Interrupt: la gestione viene passata a interrupts.c
    - TLB Refill
    - Syscall
    - Trap 

- interrupts.c: 
    Trova e risolve il corretto interrupt, distinguendo:
    - PLT interrupt
    - Interval Timer interrupt
    - Device interrupt
    Per ognuno, la risoluzione viene affidata al relativo gestore.

# Scelte progettuali
Abbiamo scelto di utilizzare un file header per contenere tutte le variabili e le funzioni extern, nonchè le macro utilizzate in tutti i file.

## initial.h
Unica scelta progettuale è la gestione dei semafori relativi ai device. Abbiamo deciso di utilizzare un singolo array di 48 (= 32 (Non Terminali) + 16 (Terminali)) semafori, inizializzati a 0.  
Il PID per il primo processo è 1.

## scheduler.h
La gestione del tempo di ogni processo viene effettuata calcolando in due momenti diversi il TOD e poi sottraendoli fra loro. Per tenere conto dell'istante in cui un processo viene caricato utilizziamo la variabile processStartTime.  
Nel caso in cui lo scheduler entri in uno stato di WAIT, disabilitiamo il PLT.
  
//PROCESSCOUNT A NULL ???????
  

## exceptions.h
Per aggiornare il tempo, usiamo una funzione ad hoc che calcola la differenza sopra citata.   
Salviamo lo stato del processore preso all'inizio della BIOSDATAPAGE in una variabile ausiliaria.   
SyscallExcHandler:
- CreateProcess:  
    Come per la fase 1, consideriamo tutti i namespace, anche se continuiamo ad avere solo NS_PID
- TerminateProcess:  
    Nel caso in cui il processo da terminare non sia il corrente utilizziamo una funzione ausiliaria per trovare il processo in base al suo PID (findPCB_pid). Questa cerca dapprima nella readyQueue e poi nelle code di tutti i semafori.  
    La funzione è ricorsiva sui figli del processo da terminare. Sfruttiamo il fatto che la nostra implementazione di removeChild sposti i successivi figli al posto del primo eliminato, rimuovendo di conseguenza solo il primo figlio ad ogni iterazione del while.  
    Se il processo da terminare è bloccato su un semaforo allora distinguiamo nei seguenti casi:  
    - Semaforo SB:  Diminuiamo il numero di processi SB.
    - Semaforo non SB: Se il processo era l'unico in coda, aggiustiamo il valore del semaforo riportandolo allo stato iniziale. //Remove riga 141
    
    Nel caso in cui il processo terminato fosse il currentProcess, non c'è più alcun processo a cui far tornare il controllo e quindi invochiamo lo scheduler.
- Passeren: 
    Implementata basandosi sulla P relativa ai semafori binari.
    Quando il valore del semaforo è a 1 e ci sono processi bloccati su di esso, la funzione rimuove il primo processo bloccato e lo inserisce nella readyQueue
- Verhogen
    Duale della P
- DO_IO:
    Per determinare il tipo didevice su cui vorremo fare la richiesta di Input / Output abbiamo realizzato le funzioni:
    - findLine: Se l'indirizzo del cmdAddress appartiene ad un device ( è compreso nella regioe dei registri destinati ai dispositivi ) allora calcola la linea dell'interrupt relativa al device.
    spiegaazione calcolo ????
    - findDevice: ritorna il subdevice relativo alla linea da cui proviene l'interrupt
    spiegazione calcolo ????
    Se il device è un non-terminale allora inserisco il primo valore dell'array cmdValues nel campo command del device in questione. Altrimenti devo distinguere due casi (ricezione e trasissione della console)
    Infine la funzione esegue un operazione P, che blocchera il processo sul semaforo relativo al device.

- waitForClock:

- getProcessID:
- getChilren:
Nel caso in cui il currentProcess abbia un figlio, la funzione inserisce nell'array passato come parametro prima il primo figlio. In seguito itera sui fratelli del primo genito. Gli inserimenti nell'array avvengono se e solo se i namespaces dei figli sono identici a quelli del padre

## interrupts.h
Gli interrupt vengono distinti in tre categorie:
- interrupt generati dal PLT: PLTinterrupt()
- interrupt generati dall' IT: ITinterrupt()
- tutti gli altri: nonTimerInterruptHandler
In particolare l'ultimo caso deve fare la distinzione tra interrupt provenienti o no dal terminale.
Per ottenere l'n-esimo bit di una stringa abbiamo implementato la macro: NBIT 
*e.g. voglio il terzo bit di: 0110 -> (1110 & 0100) >> 3*
                                            *(0100) >> 3*
                                                     *01 : il valore del terzo bit è 1*

                                             



