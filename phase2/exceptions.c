#include "pcb.h"
#include "ns.h"
#include "ash.h"
#include <pandos_const.h>
#include <pandos_types.h>
#include <umps3/umps/cp0.h>
#include <umps3/umps/arch.h>
#include <umps3/umps/libumps.h>
#include <umps3/umps/types.h>

#define ALDEV 50
extern int readyPCB;

extern int processCount;
extern int SBcount;
extern struct list_head readyQueue;
extern pcb_t* currentProcess;
extern void schedule();
extern void interruptHandler();
extern int pseudoClockSem;
extern int deviceSem[ALDEV];
extern int pseudoClockSem;
extern void P(int* sem); //dichiarato in interrupts.c

extern int processStartTime;
state_t* BIOSDPState;
int excTOD;

//TODO list
// prima di ogni schedule(), bisogna incrementare il TOD (sezione 3.8)

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
pcb_t* findPCB_pid(int pid, struct list_head* queue);
int createProcess(state_t *statep, support_t *supportp, nsd_t *ns);
void terminateProcess(int pid);
bool Passeren(int *sem);
bool Verhogen(int *sem);
int DO_IO(int *cmdAddr, int *cmdValues);
cpu_t getTime();
void waitForClock();
support_t* getSupportData();
int getProcessID(int parent);
int getChildren(int *children, int size);

void syscallExcHandler();

int PID = 1;    /* 1 is the init process */


void exceptionHandler(){
    if(currentProcess != NULL){updateCPUtime();}
    STCK(excTOD);
    /* use processor state in BIOS Data Page */
    BIOSDPState = ((state_t *) BIOSDATAPAGE);
    unsigned int excCode = CAUSE_GET_EXCCODE(BIOSDPState->cause);

    if (excCode == 0){
        interruptHandler();
    }
    else if (excCode == 8){
        syscallExcHandler();
    }
    else if (excCode < 4){  //1-2-3
        passUporDie(PGFAULTEXCEPT);
    }
    else{   /* 4-7 o 9-12 */
        passUporDie(GENERALEXCEPT);
    }
    
}

void syscallExcHandler(){

    /* increase PC to avoid loop on Syscall */
    BIOSDPState->pc_epc +=WORDLEN;
    
    /* check process if is in Kernel Mode */
    if (((getSTATUS() & STATUS_KUc)) == 2 ){
        /* set ExcCode to RI */
        BIOSDPState->cause = ((EXC_RI << CAUSE_EXCCODE_BIT));

        /*call Program Trap Handler*/
        passUporDie(GENERALEXCEPT);
    }

    else {
        /* switch on value in a0 register */

        switch (BIOSDPState->reg_a0) {

        case CREATEPROCESS:
            BIOSDPState->reg_v0 = (int) createProcess(
                (state_t*)(BIOSDPState->reg_a1),
                (support_t*)(BIOSDPState->reg_a2),
                (nsd_t*)(BIOSDPState->reg_a3));
            break;
        case TERMPROCESS:
            terminateProcess(BIOSDPState->reg_a1);
            if (BIOSDPState->reg_a1 == 0 || BIOSDPState->reg_a1 == currentProcess -> p_pid)
                schedule();
            break;
        case PASSEREN:
            if (Passeren(BIOSDPState->reg_a1)){
                currentProcess->p_s = *BIOSDPState;
                schedule();
            }
            break;
        case VERHOGEN:
            if (Verhogen(BIOSDPState->reg_a1)){
                currentProcess->p_s = *BIOSDPState;
                schedule();
            }
            break;
        case DOIO:
            BIOSDPState->reg_v0 = (int) DO_IO(
                (memaddr)(BIOSDPState->reg_a1),
                (BIOSDPState->reg_a2));
                schedule();
            break;
        case GETTIME:
            BIOSDPState->reg_v0 = (int) getTime();
            break;
        case CLOCKWAIT:
            waitForClock();
            schedule();
            break;
        case GETSUPPORTPTR:
            BIOSDPState->reg_v0 = (int) getSupportData();
            break;
        case GETPROCESSID:
            BIOSDPState->reg_v0 = (int) getProcessID(BIOSDPState->reg_a1);
            break;
        case GETCHILDREN:
            BIOSDPState->reg_v0 = (int) getChildren(
                (int*)(BIOSDPState->reg_a1),
                (*((int*)(BIOSDPState->reg_a1))));
            break;
        case 11: case 12: case 13: case 14: case 15: case 16: case 17: case 18: case 19: case 20:
            passUporDie(GENERALEXCEPT);
            break;
        }   

    /* se la syscall blocca il processo, allora è stato invocato lo scheduler, quindi non si arriverà qui */
    LDST(BIOSDPState);
    }
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
        readyPCB++;
        insertProcQ(&readyQueue, newProc);
        insertChild(currentProcess, newProc); 
        newProc->p_s = (*statep);
        newProc->p_supportStruct = supportp;
        newProc->p_time = 0;
        newProc->p_semAdd = NULL;
        newProc->p_pid = PID;

        //if ns==NULL, it fails and enters
        if(!addNamespace(newProc, ns)){ 
            /* This could be made by pointing the same structure as the father */
            for(int i=0; i<NS_TYPE_MAX; i++){
                //gives the new process the parent's namespace
                nsd_t* tmpNs = getNamespace(currentProcess, i);
                addNamespace(newProc, tmpNs); 
            }
        }
        
    }
    return PID;
}

#if 0
void killProcess(pcb_t *pcbToKill){

    if (pcbToKill -> p_semAdd != NULL){
        int *tmpSem = pcbToKill -> p_semAdd;
        outBlocked(pcbToKill);
        if(!((deviceSem <= tmpSem &&tmpSem <= deviceSem + ALDEV) || tmpSem == (&pseudoClockSem))){ 
            if(headBlocked(tmpSem)==NULL)
                *tmpSem = 1 - *tmpSem;
        }
        else{
            SBcount--;
        }
    }

    freePcb(pcbToKill);
    processCount--;
}

void killTree(pcb_t *pcbToKill){

    while(!emptyChild(pcbToKill)){
        killTree(removeChild(pcbToKill));   //rimuovo un nodo dall'albero e richiamo ricorsivo su quel nodo.
    }
    killProcess(pcbToKill);
}

void terminateProcess(int pid){
    pcb_t* toKill = (pid== 0 ||  pid == currentProcess->p_pid) ? currentProcess : findPCB_pid(pid,&readyQueue);
    outChild(toKill);       //lo scollego dal padre
    killTree(toKill);
}
#endif



void terminateProcess(int pid){
    if( pid==0 || pid == currentProcess->p_pid ) {  /* Kills the current process and progeny */
        outChild(currentProcess);
        processCount--;
        while(!emptyChild(currentProcess)){
            /* removeChild removes the first child and moves his first brother in its place:
            it only exits the loop when all the siblings have been removed */
            pcb_t* firstChild = list_first_entry(&currentProcess->p_child,struct pcb_t,p_child);
            removeChild(currentProcess);
            /*terminates the subtree of the first child*/
            terminateProcess(firstChild->p_pid);
        }
        /*each pcb is freed in its recursive call*/
        freePcb(currentProcess);
        currentProcess=NULL;
    } else {  /* Kills the pointed process and progeny */
        pcb_t* proc = findPCB_pid(pid, (&readyQueue));
        outChild(proc);
        processCount--;
        /*the process is either blocked at a semaphore or on the ready queue*/
        if(proc->p_semAdd!=NULL){
            int *tmpSem = outBlocked(proc);
            /*the semaphore is incremente only if it is not a device one*/
            if(!((deviceSem <= tmpSem && tmpSem <= deviceSem + ALDEV)||tmpSem == &pseudoClockSem)){ // *sizeof(int) dovrebbe essere implicito in C
                if (headBlocked(tmpSem)==0){
                    *(tmpSem) =1 - *(tmpSem);
                }                
            }
            else{
                SBcount--;
            }
        } else { 
            outProcQ(&readyQueue, proc);
            readyPCB--;
        }

        while(!emptyChild(proc)){
            /* removeChild removes the first child and moves his first brother in its place:
            it only exits the loop when all the siblings have been removed */
            pcb_t* firstChild = list_first_entry(&proc->p_child,struct pcb_t, p_child);
            removeChild(proc);
            /*terminates the subtree of the first child*/
            terminateProcess(firstChild->p_pid);
       }
       freePcb(proc);
    }
    if (currentProcess==NULL){
        schedule();
    }

} 


bool Passeren(int* sem){

    if (*sem == 0){     /* blocked */
        /* current process enters in block state */
        insertBlocked(sem,currentProcess);
        return true;
    }
   
    else if (headBlocked(sem) != NULL){   /* more pcb in sem queue */
        insertProcQ(&readyQueue,removeBlocked(sem));
        readyPCB++;
        return false; 

    }
    
    else{      /* there is NO semaphore -> no PCB */ 
        (*sem) = (*sem)-1;
        return false;
    }
}

bool Verhogen(int* sem){
    if((*sem) == 1){
        insertBlocked(sem,currentProcess);
        return true;
    }

    else if (headBlocked(sem) != NULL){
        insertProcQ(&readyQueue,removeBlocked(sem));
        readyPCB++;
        return false;
    }
    else{
        (*sem) = (*sem) + 1;
        return false;
    }

}


/* Effettua un’operazione di I/O. */
int DO_IO(int *cmdAddr, int *cmdValues){
    currentProcess->valueAddr = cmdValues;
    
    /* 
    I device si trovano a 
        0x1000.0054 - 0x1000.0063 Line 3, Device 0 (Device Register)
        0x1000.0064 - 0x1000.0073 Line 3, Device 1 (Device Register)
        ...
        0x1000.00C4 - 0x1000.00D3 Line 3, device 7 (Device Register)

        0x1000.00D4 - 0x1000.0153 Line 4, device 0-7 (Device Register)
        0x1000.0154 - 0x1000.01D3 Line 5, device 0-7 (Device Register)
        0x1000.01D4 - 0x1000.0253 Line 6, device 0-7 (Device Register)
        0.1000.0254 - 0x1000.02D3 Line 7, device 0-7 (Device Register)

    Disk devices
    Flash devices
    Network Adapters
    Printer devices
    Terminal devices (receive & transmit)

    Base + 0x4 è dove mettere il comando (non per il terminale)

    Installed device bitmap ci dice a quali interrupt sono collegati i device
    Interrupt device bitmap, come sopra ma dice quale interrupt è attivo
    */

    /*Find the device function*/
    //int indexDevice = findDevice(cmdAddr);
    //temporaneo:
    int indexDevice=findDevice(cmdAddr);

    /*Write command Values from command Address */
        /*devreg.dtp.command oppure 
        devreg.term.recv_command/devreg.term.transm_command*/

    if (indexDevice<0){
        return -1;
    }
    else if (indexDevice < 34){
        /*non-terminal*/
        dtpreg_t* regdevice = (dtpreg_t*)(cmdValues);
        cmdAddr = regdevice;
    }
    else {
        /*terminal*/
        termreg_t* terminal;
        if ((unsigned int)cmdAddr % 16 == 4 ){
            terminal = cmdAddr;
            terminal->recv_command = cmdValues[0]; 
        }
        else {
            /* cmdAddr is the trasm  address, so the terminal is cmdAddress - 8*/
            terminal = (unsigned int)cmdAddr - 8;
            terminal -> transm_command = cmdValues[0];
        } 
    }

    
    /*Block the process on that device */
    int* sem = (deviceSem+indexDevice);
    currentProcess->p_s = *BIOSDPState;
    P(sem);
/*  
    At the completion of the I-O operation the device register values 
    are copied back in the cmdValues array. The return value of this system 
    call is 0 in case of success, -1 otherwise.
    Dobbiamo ritornare qualcosa?
*/
    return 0;
}


/* NON FUNGE */
//si possono modificare le etichette ai semafori ma segnalo così cambiamo anche il resto :)
int findDevice(int *cmdAddr){
    /*
    All Lines Devices:

    0 -> PLT
    1 -> Interval Timer
    2...9 -> Disk Devices
    10...17 -> Flash Devices
    18...25 -> Network Devices
    26...33 -> Printer Devices
    34...41 -> Terminal Devices

    DEV_IL_START = 3;
    */

    /* DEV_REG_ADDR ci restituisce l'indirizzo della linea IL_*, del device 0-N_DEV_PER_IL*/
    int value = cmdAddr; //è la base
    if (value < (memaddr)DEV_REG_START || value >= (memaddr) DEV_REG_END){
        return -1;
    }
    else if (value >= (memaddr) DEV_REG_ADDR(IL_DISK, 0) && value < (memaddr)DEV_REG_ADDR(IL_DISK, N_DEV_PER_IL)){
        /*disk device*/
        return  ((128-(DEV_REG_ADDR(IL_DISK, N_DEV_PER_IL)-value))/16)+2; 
    }
    else if (value >= (memaddr)DEV_REG_ADDR(IL_FLASH, 0) && value < (memaddr) DEV_REG_ADDR(IL_FLASH, N_DEV_PER_IL)){
        /*flash device*/
        return  ((128-(DEV_REG_ADDR(IL_FLASH, N_DEV_PER_IL)-value))/16)+10;
    }
    else if (value >= (memaddr)DEV_REG_ADDR(IL_ETHERNET, 0) && value < (memaddr) DEV_REG_ADDR(IL_ETHERNET, N_DEV_PER_IL)){
        /*network */
        return  ((128-(DEV_REG_ADDR(IL_ETHERNET, N_DEV_PER_IL)-value))/16)+18;
    }
    else if (value >= (memaddr)DEV_REG_ADDR(IL_PRINTER, 0) && value < (memaddr) DEV_REG_ADDR(IL_PRINTER, N_DEV_PER_IL)){
        /*Printer*/
        return ((128-(DEV_REG_ADDR(IL_PRINTER, N_DEV_PER_IL)-value))/16)+26;
    }
    else if (value >= (memaddr)DEV_REG_ADDR(IL_TERMINAL, 0) && value < (memaddr) DEV_REG_ADDR(IL_TERMINAL, N_DEV_PER_IL)){
        /*terminal*/
        return ((128-(DEV_REG_ADDR(IL_TERMINAL, N_DEV_PER_IL)-value))/8)+34;
    }
}
/*Restituisce il tempo di esecuzione (in microsecondi, quindi *1000?) del processo */
cpu_t getTime(){
    return (currentProcess->p_time); /* v0 inizializzata dopo*/
}

void waitForClock(){
    if(pseudoClockSem == 0){
    /* Always block on Psuedo-clock sem */
    /* increase PC to avoid loop on Syscall */
    currentProcess->p_s = *BIOSDPState;
    /* current process enters in block state */
    insertBlocked(&pseudoClockSem, currentProcess);
    SBcount++;
    }
    else 
        pseudoClockSem -- ;
    
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
            return currentProcess-> p_parent-> p_pid;
        }
        return 0;
    }
    else return currentProcess->p_pid;
}

/*Deve ritornare il numero di figli con lo stesso namespace */
int getChildren(int* children, int size){
    int valueToReturn = 0;
    pcb_t* firstChild = list_first_entry(&currentProcess->p_child,struct pcb_t,p_child);
    
    if (!emptyChild(currentProcess)){                            // controllo se il pcb ha figli
        nsd_t* currentNs = getNamespace(currentProcess, NS_PID);
        struct pcb_t* iterator = NULL;
        
        list_for_each_entry(iterator,&firstChild->p_sib,p_sib){
            if (currentNs == getNamespace(iterator, NS_PID)){   
                if (size < valueToReturn)
                    *(children + valueToReturn) = iterator->p_pid;// finche riesco assegno alla cella contigua dell'array il pid del processo figlio 
                valueToReturn ++;
            }
        }
    }
    return valueToReturn;
}

void passUporDie(int indexValue){
    if (currentProcess->p_supportStruct == NULL){
        /* Die part */
        terminateProcess(currentProcess->p_pid);
    }
    else{
        /*Passup part*/
        currentProcess->p_supportStruct->sup_exceptState[indexValue]=(*(state_t*)(BIOSDATAPAGE));
        LDCXT(currentProcess->p_supportStruct->sup_exceptContext[indexValue].stackPtr,
              currentProcess->p_supportStruct->sup_exceptContext[indexValue].status, 
              currentProcess->p_supportStruct->sup_exceptContext[indexValue].pc);
    }
}

void updateCPUtime(){
    unsigned int tod;
    STCK(tod);
    currentProcess->p_time += (cpu_t)(tod - processStartTime);
}
