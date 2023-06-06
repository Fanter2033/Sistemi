#include "exceptions.h"

void exceptionHandler(){
    if(currentProcess != NULL){updateCPUtime();}
    /* use processor state in BIOS Data Page */
    BIOSDPState = ((state_t *) BIOSDATAPAGE);
    unsigned int excCode = CAUSE_GET_EXCCODE(BIOSDPState->cause);

    if (excCode == 0){
        interruptHandler();
    }
    else if (excCode == 8){
        syscallExcHandler();
    }
    else if (excCode < 4){  /*1-2-3*/
        passUporDie(PGFAULTEXCEPT);
    }
    else{   /* 4-7 o 9-12 */
        passUporDie(GENERALEXCEPT);
    }
    
}

void syscallExcHandler(){

    /* increase PC to avoid loop on Syscall */
    BIOSDPState->pc_epc +=WORDLEN;
    
    /* check if process is in Kernel Mode */
    if (((getSTATUS() & STATUS_KUc)) == 2 ){
        /* set ExcCode to RI */
        BIOSDPState->cause = ((EXC_RI << CAUSE_EXCCODE_BIT));
        /* call Program Trap Handler */
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
            if (BIOSDPState->reg_a1 == 0 || BIOSDPState->reg_a1 == currentProcess -> p_pid) //Il secondo controllo è inutile, il currentProcess è NULL qui
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
                ((int)(BIOSDPState->reg_a1)));
            break;
        default: 
            passUporDie(GENERALEXCEPT);
            break;
        }   

    /*if any syscall has blocked the process,the scheduler
     must've been invoked, so we shouldn't arrive here */
    LDST(BIOSDPState);
    }
}


int createProcess(state_t *statep, support_t *supportp, nsd_t *ns){
    pcb_t* newProc = allocPcb();
    /*no more free PCB to assign*/
    if (newProc == NULL){ 
        return 0;
    } else {
        PID++;
        processCount++;
        /* the new process is set to be the 
        child of the current one and added to the readyQueue */
        insertProcQ(&readyQueue, newProc);
        insertChild(currentProcess, newProc); 
        newProc->p_s = (*statep);
        newProc->p_supportStruct = supportp;
        newProc->p_time = 0;
        newProc->p_semAdd = NULL;
        newProc->p_pid = PID;

        /* if the namespace is null, it fails and enters */
        if(!addNamespace(newProc, ns)){ 
            /* this could be made by pointing the same structure as the father */
            for(int i=0; i<NS_TYPE_MAX; i++){
                /* gives the new process the parent's namespace */
                nsd_t* tmpNs = getNamespace(currentProcess, i);
                addNamespace(newProc, tmpNs); 
            }
        }
        
    }
    return PID;
}

void terminateProcess(int pid){
    if( pid==0 || pid == currentProcess->p_pid ) { /* kills the current process and progeny */
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
    } else {  /* kills the pointed process and progeny */
        pcb_t* proc = findPCB_pid(pid, (&readyQueue));
        outChild(proc);
        processCount--;
        /* the process is either blocked at a semaphore or on the ready queue*/
        if(proc->p_semAdd!=NULL){
            int *tmpSem = outBlocked(proc);
            /*the semaphore is incremented only if it is not a soft blocking one*/
            if(!((deviceSem <= tmpSem && tmpSem <= deviceSem + ALDEV) || tmpSem == &pseudoClockSem)){ 
                if (headBlocked(tmpSem)==0){
                    *(tmpSem) = 1 - *(tmpSem);
                }            
            }
            else{
                SBcount--;
            }
        } else { 
            outProcQ(&readyQueue, proc);
        }

        while(!emptyChild(proc)){
            /* removeChild removes the first child and moves his first brother in its place:
            it only exits the loop when all the siblings have been removed */
            pcb_t* firstChild = list_first_entry(&proc->p_child,struct pcb_t, p_child);
            removeChild(proc);
            /* terminates the subtree of the first child*/
            terminateProcess(firstChild->p_pid);
       }
       freePcb(proc);
    }
    if (currentProcess==NULL){
        schedule();
    }

} 


bool Passeren(int* sem){

    if (*sem == 0){     /* blocking */
        /* current process enters in block state */
        insertBlocked(sem,currentProcess);
        return true;
    }
   
    else if (headBlocked(sem) != NULL){   /* more pcb in sem queue */
        insertProcQ(&readyQueue,removeBlocked(sem));
        return false; 
    }
    
    else{      /* there is NO semaphore -> no PCB */ 
        (*sem) = 0;
        return false;
    }
}

bool Verhogen(int* sem){
    if((*sem) == 1){    /* blocking */
        /* current process enters in block state*/
        insertBlocked(sem,currentProcess);
        return true;
    }
    
    else if (headBlocked(sem) != NULL){    /* more pcb in sem queue */
        insertProcQ(&readyQueue,removeBlocked(sem));
        return false;
    }
    else{    /* there is NO semaphore -> no PCB */ 
        (*sem) = 1;
        return false;
    }
}


/* performs an input output operation*/
int DO_IO(int *cmdAddr, int *cmdValues){
    currentProcess->valueAddr = cmdValues;
    
    /* 
    I device si trovano a:
        0x1000.0054 - 0x1000.0063 Line 3, Device 0 (Device Register)
        0x1000.0064 - 0x1000.0073 Line 3, Device 1 (Device Register)
        ...
        0x1000.00C4 - 0x1000.00D3 Line 3, device 7 (Device Register)

        0x1000.00D4 - 0x1000.0153 Line 4, device 0-7 (Device Register)
        0x1000.0154 - 0x1000.01D3 Line 5, device 0-7 (Device Register)
        0x1000.01D4 - 0x1000.0253 Line 6, device 0-7 (Device Register)
        0.1000.0254 - 0x1000.02D3 Line 7, device 0-7 (Device Register)

    Disk, Flash, Network, Printer, Terminal (receive & transmit)
    Base + 0x4 è dove mettere il comando (non per il terminale)
    */

    int line = findLine(cmdAddr);
    int device = findDevice(line,cmdAddr);

    int indexDevice = (EXT_IL_INDEX(line)*DEVPERINT)+ device;


    /*Write command Values from command Address */

    if (indexDevice<0){
        return -1;
    }
    else if (indexDevice < 32){
        /*Non-terminal*/
        dtpreg_t* regdevice = (dtpreg_t*)(cmdValues);
        cmdAddr = regdevice;
    }
    else {
        /*Terminal*/
        termreg_t* terminal;
        if ((unsigned int)cmdAddr % 16 == 4 ){
            terminal = cmdAddr;
            terminal->recv_command = cmdValues[1]; 
        }
        else {
            /* cmdAddr is the trasm address, so the terminal is cmdAddress - 8*/
            terminal = (unsigned int)cmdAddr - 8;
            terminal -> transm_command = cmdValues[1];
        } 
    }

    
    /*Block the process on that device */
    int sem = (&deviceSem[indexDevice]);
    currentProcess->p_s = *BIOSDPState;
    P((int*)sem);

    return 0;
}


int findLine(int *cmdAddr){
    if (cmdAddr < (memaddr)DEV_REG_START || cmdAddr >= (memaddr) DEV_REG_END){
            return -1;
    }
    else
        return (((int)cmdAddr - (int)DEV_REG_START) / (DEV_REG_SIZE*DEVPERINT)) + DEV_IL_START;

}

int findDevice(int line,int* cmdAddr){
    return line == IL_TERMINAL ? (int)((int)cmdAddr - (int)DEV_REG_ADDR(line,0)) / DEVPERINT : (int)((int)cmdAddr - (int)DEV_REG_ADDR(line,0)) / (DEVPERINT*2); 
}


/* returns the executing time of the process */
cpu_t getTime(){
    return (currentProcess->p_time);
}

void waitForClock(){
    if(pseudoClockSem == 0){
        /* Always block on Psuedo-clock sem */
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

/*returns the identifier of the invoking process if parent == 0, 
  the invoking process' parent one otherwise */

int getProcessID(int parent){
    /* type=0 is the PID namespace, currently using p_pid */
    nsd_t* ns = getNamespace(currentProcess, NS_PID);
    if (parent){
        if (ns==getNamespace(currentProcess->p_parent,NS_PID)){
            return currentProcess-> p_parent-> p_pid;
        }
        return 0;
    }
    else return currentProcess->p_pid;
}



/* Returns the number of children within the same namespace */
int getChildren(int* children, int size){
    int valueToReturn = 0;
    if (!emptyChild(currentProcess)){                            /* check if pcb has children*/
        pcb_t* firstChild = list_first_entry(&currentProcess->p_child,struct pcb_t,p_child);
        nsd_t* currentNs = getNamespace(currentProcess, NS_PID);
        if (currentNs == getNamespace(firstChild, NS_PID)){   
            if (valueToReturn < size)
                *(children + valueToReturn) = firstChild->p_pid; //controllo se il primo figlio è da inserire 
            valueToReturn ++;
        }

        struct pcb_t* iterator = NULL;
        list_for_each_entry(iterator,&firstChild->p_sib,p_sib){
            if (currentNs == getNamespace(iterator, NS_PID)){   
                if (valueToReturn < size)
                    *(children + valueToReturn) = iterator->p_pid; /* Finchè riesco assegno alla cella contigua dell'array il pid del processo figlio */ 
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

void *memcpy(void *dest, const void *src, unsigned long n)
{
    for (unsigned long i = 0; i < n; i++)
    {
        ((char*)dest)[i] = ((char*)src)[i];
    }
}