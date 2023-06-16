# Descrizione progetto
Lo scopo della fase 2 del progetto è la realizzazione del livello 3 del S.O. Panda+, ovvero la parte del kernel, in particolare:
- Inizializzazione del sistema 
- Scheduling dei processi 
- Gestione delle eccezioni: 
    - Syscalls
    - Interrupts
    - Traps  
<br>

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
    - Interrupt
    - TLB Refill
    - Syscall
    - Trap  

- interrupts.c:   
    Trova e risolve il corretto interrupt, distinguendo:
    - PLT interrupt
    - Interval Timer interrupt
    - Device interrupt

    Per ognuno, la risoluzione viene affidata al relativo gestore.  
<br> 

# Scelte progettuali
Abbiamo scelto di utilizzare un file header per contenere tutte le variabili e le funzioni extern, nonchè le macro utilizzate in tutti i file (utils.h).

## initial.h
Unica scelta progettuale è la gestione dei semafori relativi ai device. Abbiamo deciso di utilizzare un singolo array di 48 (= 32 (Non Terminali) + 16 (Terminali)) semafori, inizializzati a 0. Di conseguenza la parte receive dei terminali è pari, la transmit è dispari.  
Il PID per il primo processo è 1.

## scheduler.h
La gestione del tempo di ogni processo viene effettuata calcolando in due momenti diversi il TOD e poi sottraendoli fra loro. Per tenere conto dell'istante in cui un processo viene caricato utilizziamo la variabile processStartTime.  
Nel caso in cui lo scheduler entri in uno stato di WAIT, disabilitiamo il PLT.  
Il currentProcess viene settato a NULL in quanto così sapremo, nel caso avvenga un interrupt mentre siamo all'interno dello scheduler, che non dobbiamo far tornare il controllo a un processo, ma deve essere invocato lo scheduler stesso.

## exceptions.h
Per aggiornare il tempo, usiamo una funzione ad hoc che calcola la differenza sopra citata.   
Salviamo lo stato del processore preso all'inizio della BIOSDATAPAGE in una variabile ausiliaria (BIOSDPSTATE).  
Ogni eccezione viene affidata al gestore corretto:
- Interrupt: interruptHandler()
- TLBrefill: passUporDie(PGFAULT)
- Syscall: SyscallExcHandler()
- Traps: PassUporDie(GENERAL)

### SyscallExcHandler:

- CreateProcess:  
    Come per la fase 1, consideriamo tutti i namespace, anche se continuiamo ad avere solo NS_PID.
- TerminateProcess:  
    Nel caso in cui il processo da terminare non sia il corrente utilizziamo una funzione ausiliaria per trovare il processo in base al suo PID (findPCB_pid). Questa cerca dapprima nella readyQueue e poi nelle code di tutti i semafori. 
    L'abbiamo implementata nei file (ash.c e pcb.c) della fase 1.  
    La funzione è ricorsiva sui figli del processo da terminare. Sfruttiamo il fatto che la nostra implementazione di removeChild sposti i successivi figli al posto del primo eliminato, rimuovendo di conseguenza solo il primo figlio ad ogni iterazione del while.  
    Se il processo da terminare è bloccato su un semaforo dei device, diminuiamo il numero di processi SB.  
    Nel caso in cui il processo terminato fosse il currentProcess, non c'è più alcun processo a cui far tornare il controllo e quindi invochiamo lo scheduler.
- Passeren:  
    Implementata basandosi sulla P relativa ai semafori binari.  
    Se il valore del semaforo è a 0, allora blocchiamo il processo, altrimenti se il valore del semaforo è a 1 e ci sono processi bloccati su di esso, la funzione rimuove il primo processo bloccato e lo inserisce nella readyQueue.  
    N.B.: Il valore del semaforo non corrisponde al numero di processi bloccati, dunque nella Terminate non va aggiustato. 
- Verhogen:  
    Duale della Passeren.
- DO_IO:  
    Per salvare l'indirizzo dell'array cmdValues abbiamo aggiunto un campo alla struttura pcb_t in pandos_types.h. Questo ci servirà nella gestione degli interrupt per salvare il valore di ritorno di DO_IO.

    Per determinare il tipo di device su cui vorremo fare la richiesta di I/O abbiamo realizzato le funzioni:
    - findLine: Se l'indirizzo del cmdAddress appartiene ad un device (è compreso nella regione dei registri destinati ai dispositivi) allora calcola la linea dell'interrupt relativa al device.  
    
    > Esempio:   
     cmdAddr = 0x1000254 (Terminale, primo (sub)device)  
     cmdAddr - 0x1000054 = 512  
     512/128 = 4  
     4 + 3 = 7 --> Linea corretta

    - findDevice: ritorna il subdevice relativo alla linea di interrupt 

    > Esempio:  
     cmdAddr = 0x1000254 (Terminale, primo (sub)device)  
     linea = 7  --> inizio terminali = 0x1000254
     cmdAddr - 0x1000254 = 0  
     0/8 = 0  --> Numero (sub)device corretto

    Allora l'indice nell'array dei semafori viene calcolato così:  

    >Esempio:  
     cmdAddr = 0x1000254 (Terminale, primo (sub)device)  
     linea = 7, device = 0  
     index = (7-3)*8 + 0 = 32 --> device mappato correttamente

    Se il device è un non-terminale allora inseriamo il primo valore dell'array cmdValues nel campo command del device in questione, altrimenti distinguiamo due casi (ricezione e trasmissione).
    Infine la funzione esegue un operazione P (interrupts.h), che bloccherà il processo sul semaforo del relativo device.

- getChilren:  
    Nel caso in cui il currentProcess abbia un figlio, la funzione inserisce nell'array passato come parametro il primo figlio. In seguito itera sui fratelli del primogenito. Gli inserimenti nell'array avvengono se e solo se i PID namespace dei figli sono identici a quelli del padre e c'è abbastanza spazio.  

Alcune syscall possono bloccare o meno, quindi ritornano un valore true o false per permetterci di identificare i due casi.

Memcopy:  
    Questa funzione viene utilizzata per copiare n caratteri da un sorgente a una destinazione. Solitamente è inclusa nella libreria stdio.h che non includiamo.

## interrupts.h

Per trovare la linea dell'interrupt (o più interrupt) attivo facciamo un for da 1 al numero di linee in modo da considerare la priorità. Per trovarla usiamo NBIT.

NBIT è una macro che fornisce l'n+1-esimo bit (da destra) di una stringa:
> Esempio:  
Voglio il terzo bit di: 1110  
(1110 & (1 << 2)) >> 2  
(1110 & 100) >> 2  
0100 >> 2 = 01  
Il valore del terzo bit è 1 

Gli interrupt vengono distinti in tre categorie:
- interrupt generati dal PLT (linea 1): PLTinterrupt() 
- interrupt generati dall' IT (linea 2): ITinterrupt()
- tutti gli altri (linee 3-7): nonTimerInterruptHandler(line)

L'ultimo caso deve fare la distinzione tra interrupt provenienti dal terminale, o da altri device.  
NBIT qui viene utilizzata per risalire al device che ha dato origine all'interrupt, se il device appartiene alla settima linea viene chiamata la funzione resolveTerm altrimenti resolveNonTerm.

- resolveTerm: distingue i casi, transmitted (priorità più alta) e received (priorità più bassa). In entrambi i casi viene salvato lo stato e mandato l'ACK. Viene infine chiamata la procedura unlockPCB a cui viene passato l'indice del singolo device che ha generato l'interrupt.

- resolveNonTerm: stesso funzionamento della resolveTerm ma senza la distinzione tra transmitted e received.

- UnlockPCB: funzione ausiliaria per resolveTerm e resolveNonTerm, 
effettua un'operazione V (sbloccante sul semaforo) e inserisce il processo 
nella coda ready.
                                             
P, V:   
Abbiamo implementato dei semafori semplici per gestire l'accesso esclusivo ai device.

# Compilazione
Per compilare il progetto è sufficiente utilizzare ` make `.  
Per eliminare i file creati dal make si utilizza ` make clean `.

I file riguardanti umps vengono creati nella cartella machine. Dunque per creare una 
nuova configurazione su umps3 selezionare la directory "machine" . 
