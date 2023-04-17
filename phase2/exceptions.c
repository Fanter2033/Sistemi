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

void *memcpy(void *dest, const void *src, unsigned long n)
{
    for (unsigned long i = 0; i < n; i++)
    {
        ((char*)dest)[i] = ((char*)src)[i];
    }
}

pcb_t* findPCBfromQUEUE(int pid, struct list_head* head );
pcb_t* findPCB_pid(int pid);
int createProcess(state_t *statep, support_t *supportp, nsd_t *ns);
void terminateProcess(int pid);
void Passeren();
void Verhogen();
int DO_IO(int *cmdAddr, int *cmdValues);
cpu_t getTime();
void waitForClock();
support_t* getSupportData();
int getProcessID(int parent);
int getChildren(int *children, int size);

void syscallExcHandler();

int PID = 2;    /* 1 is the init process */


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
    currentProcess->p_s.pc_epc += WORDLEN;
    /* take value frome a0 register */
    unsigned int a0 = (*((int *)currentProcess ->p_s.reg_a0));

    switch (a0) {
    case CREATEPROCESS:
        currentProcess->p_s.reg_v0 = (int) createProcess((state_t*)(currentProcess ->p_s.reg_a1),(support_t*)(currentProcess ->p_s.reg_a2),(nsd_t*)(currentProcess ->p_s.reg_a3));
        break;
    case TERMPROCESS:
        terminateProcess((int*)(currentProcess ->p_s.reg_a1));
        break;
    case PASSEREN:
        Passeren();
        break;
    case VERHOGEN:
        Verhogen();
        break;
    case DOIO:
        currentProcess->p_s.reg_v0 = (int) DO_IO((int*)(currentProcess ->p_s.reg_a1),(int*)(currentProcess ->p_s.reg_a2));
        break;
    case GETTIME:
        currentProcess->p_s.reg_v0 = (int) getTime();
        break;
    case CLOCKWAIT:
        waitForClock();
        break;
    case GETSUPPORTPTR:
        currentProcess->p_s.reg_v0 = (int) getSupportData();
        break;
    case GETPROCESSID:
        currentProcess->p_s.reg_v0 = (int) getProcessID((int*)(currentProcess ->p_s.reg_a1));
        break;
    case GETCHILDREN:
        currentProcess->p_s.reg_v0 = (int) getChildren((int*)(currentProcess ->p_s.reg_a1),(*((int*)(currentProcess ->p_s.reg_a1))));
        break;
    
    default:        // > 11

        break;
    }   

    /* se la syscall blocca il processo, allora è stato invocato lo scheduler, quindi non si arriverà qui */
    LDST(&(currentProcess->p_s));
}


int createProcess(state_t *statep, support_t *supportp, nsd_t *ns){
    pcb_t* newProc = allocPcb();
    //there are no more free pcbs
    if (newProc == NULL){ 
        return 0;
    } else {
        PID++;
        processCount++;
        /* The new process is set to be the 
        child of the current one and added to the readyQueue */
        insertProcQ(readyQueue, newProc);
        insertChild(currentProcess, newProc); 
        newProc->p_s = (*statep);
        newProc->p_supportStruct = supportp;
        newProc->p_time = 0;
        newProc->p_semAdd = NULL;
        //if ns==NULL, it fails and enters
        if(!addNamespace(newProc, ns)){ 
            /* This could be made by pointing the same structure as the father */
            for(int i=0; i<NS_TYPE_MAX; i++){
                //gives the new process the parent's namespace
                nsd_t* tmpNs = getNamespace(currentProcess, i);
                addNamespace(newProc, tmpNs); 
            }
        }
        newProc->p_pid = PID;
    }
    return PID;
}

void terminateProcess(int pid){
    if(pid==0){  /* Kills the current process and progeny */
        outChild(currentProcess);
        processCount--;
        if(!emptyChild(currentProcess)){
            /* We should also kill all the siblings */
            pcb_t* firstChild = list_first_entry(&currentProcess->p_child,struct pcb_t,p_child);
            terminateProcess(firstChild->p_pid);
        }
        freePcb(currentProcess);
    } else {  /* Kills the pointed process and progeny */
    

        pcb_t* proc = findPCB_pid(pid);
        outChild(proc);
        processCount--;
        //the process is either blocked at a semaphore or on the ready queue
        if(proc->p_semAdd!=NULL){
            proc->p_semAdd +=1; //? see page 24 of phase2.book
            outBlocked(proc);
            SBcount--;
            //device semaphore?
        } else { 
            outProcQ(readyQueue, proc);
        }
        if(!emptyChild(proc)){
            pcb_t* firstChild = list_first_entry(&currentProcess->p_child,struct pcb_t,p_child);
            terminateProcess(firstChild->p_pid);
       }
    }
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

/* Effettua un’operazione di I/O. */
int DO_IO(int *cmdAddr, int *cmdValues){
    
    return 0;
}

/*Restituisce il tempo di esecuzione (in microsecondi, quindi *1000?) del processo */
cpu_t getTime(){
    return currentProcess->p_time; /* v0 inizializzata dopo*/
}

void waitForClock(){
    /* Always block on Psuedo-clock sem */
    insertBlocked(&pseudoClockSem, currentProcess);
    schedule();
    /* sezione 3.6.3 per le V dello pseudo clock semaphore, da gestire nell'interrupt handler*/
}

support_t* getSupportData(){
    return currentProcess->p_supportStruct;
}

/*Restituisce l’identificatore del processo invocante se parent == 0, 
  quello del genitore del processo invocante altrimenti.*/

int getProcessID(int parent){
    /*type=0 fa riferimento al PID, usiamo 
    p_pid finchè non carica i file giusti */
    /* Si potrebbe estendere a tutti i tipi, dipende da cosa 
    facciamo nella create process */
    nsd_t* ns = getNamespace(currentProcess, NS_PID);
    if (parent){
        if (ns==getNamespace(currentProcess->p_parent,NS_PID)){
            return currentProcess->p_pid;
        }
        return 0;
    }
    else return currentProcess->p_pid;
}

/*Deve ritornare il numero di figli con lo stesso PID (?)*/
int getChildren(int* children, int size){

}


pcb_t* findPCB_pid(int pid){
    pcb_t* PCBToReturn;

    /* search in ready queue */
    PCBToReturn = findPCBfromQUEUE(pid, readyQueue);

    /* search in semaphores */
    if(PCBToReturn != NULL){
        int bkt=0;
        struct semd_t* iterator;
        hash_for_each(semd_h,bkt,iterator,s_link){
            PCBToReturn = findPCBfromQUEUE(pid,&iterator->s_procq);
            if (PCBToReturn != NULL)
                return PCBToReturn;
        }
    }
    return PCBToReturn;
}

pcb_t* findPCBfromQUEUE(int pid, struct list_head* head ){
    pcb_t* iterator = NULL;
    list_for_each_entry(iterator,head,p_list){
        if (iterator->p_pid == pid)      // p is in the list
            return iterator;
    }
    return NULL;
}