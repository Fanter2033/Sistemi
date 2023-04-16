#include "pcb.h"
#include "ns.h"
#include "ash.h"
#include <pandos_const.h>
#include <pandos_types.h>
#include <umps3/umps/cp0.h>
#include <umps3/umps/libumps.h>


extern int processCount;
extern int SBcount;
extern struct list_head* readyQueue;
extern pcb_t* currentProcess;
extern void schedule();
extern int pseudoClockSem;
// dichiaro le funzioni per non avere problemi nel make
// si potranno togliere con la distinzione .h e .c

void Passeren();
void waitForClock();
void syscallExcHandler();

/*
    per tenere traccia dei PID, io userei un semplice intero progressivo
*/


void exceptionHandler(){

    /* save processor state from BIOS Data Page */
    currentProcess->p_s = (*((state_t *) BIOSDATAPAGE)); 
    //dà un problema con il make, si dovrebbe provare a togliere il -nosdtlib ma non risolve
    //memcpy() è definita in string.h, da capire se dobbiamo implementarla noi (su stackoverflow c'è)
    //o possiamo linkare string.h

    unsigned int excCode = CAUSE_GET_EXCCODE(getCAUSE()); // from CPU or PCB ?? 

    if (excCode == 0){
        //controllo passa all'Interrupt Handler
    }
    else if (excCode == 8){
        syscallExcHandler();
    }
    else if (excCode < 4){  //1-2-3
        //controllo passa al TLB Exception Handler (pass up or die)
    }
    else{   /* 4-7 o 9-12 */
        //controllo passa al Program Trap Exception Handling (pass up or die)
    }
}

void syscallExcHandler(){
    /* increase PC to avoid loop on Syscall */
    currentProcess->p_s.pc_epc += 0x00000004;
    /* take value frome a0 register */
    unsigned int a0 = (*((int *)currentProcess ->p_s.reg_a0));

    switch (a0) {
    case CREATEPROCESS:
        //createProcess(a1,a2,a3);
        break;
    case TERMPROCESS:
        //terminateProcess(a1);
        break;
    case PASSEREN:
        Passeren();
        break;
    case VERHOGEN:
        //Verhogen(a1);
        break;
    case IOWAIT:
        //DO_IO(a1,a2);
        break;
    case GETTIME:
        //getCpuTime();
        break;
    case CLOCKWAIT:
        waitForClock();
        break;
    case GETSUPPORTPTR:
        //getSupportData();
        break;
    case TERMINATE:
        //getProcessID(a1);
        break;
    case GET_TOD:
        //getChildren(a1,a2);
        break;
    
    default:        // > 11

        break;
    }   

    /* se la syscall blocca il processo, allora è stato invocato lo scheduler, quindi non si arriverà qui */
    LDST(&(currentProcess->p_s));
}


void Passeren(){
    int* sem = ((int *)currentProcess->p_s.reg_a1);
    if (*sem == 0){     /* blocked */
        insertBlocked(sem,currentProcess);  // output -> 0 o 1, come mi comporto ??
        schedule(); /* suspend currentProcess */
    }
    else if (headBlocked == NULL){      /* there is NO semaphore -> no PCB */
        *sem-- ;
        
    }
    else{       /* resource available, more pcb in sem queue */
        insertBlocked(sem,currentProcess);  
        /* SCEGLIERE SE RIATTIVARE IL PROCESSO "LIBERATO" O METTERLO NELLA READY QUEUE */
        /* per ora lo inserisco in ready queue e chiamo lo scheduler */
        insertProcQ(readyQueue,removeBlocked);
        schedule();
    }
}

void waitForClock(){
    /* always block on Psuedo-clock sem */
    insertBlocked(&pseudoClockSem, currentProcess);
    schedule();
    /* sezione 3.6.3 per le V dello pseudo clock semaphore, da gestire nell'interrupt handler*/
}

/* Effettua un’operazione di I/O. */
int DO_IO(int *cmdAddr, int *cmdValues){
    
    return 0;
}

/*Restituisce il tempo di esecuzione (in microsecondi, quindi *1000?) del processo */
cpu_t getTime(){
    return currentProcess->p_time; /* v0 inizializzata dopo*/
}

/*Restituisce l’identificatore del processo invocante se parent == 0, 
  quello del genitore del processo invocante altrimenti.*/

int getProcessID(int parent){
    /*type=0 fa riferimento al PID, usiamo 
    p_pid finchè non carica i file giusti */
    nsd_t* ns = getNamespace(currentProcess, NS_PID);
    if (parent){
        if (ns==getNamespace(currentProcess->p_parent,NS_PID)){
            return currentProcess->p_pid;
        }
        return 0;
    }
    else return currentProcess->p_pid;
}

void Verhogen(){
        int *semaddr = (*(int* ) (currentProcess->p_s.reg_a1)) ;
        int sem_value = *semaddr ;

        if(sem_value == 1){
            /* in teoria non deve succedere nulla */
        }

        else if (headBlocked(semaddr) != NULL)
            removeBlocked(semaddr);
        else
            sem_value++;
    }

support_t* GetSupportData(){
    return currentProcess->p_supportStruct;
}

