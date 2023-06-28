#include "exceptions.h"

void exceptionHandler(){
    if(currentProcess != NULL){updateCPUtime();}
    BIOSDPState = ((state_t *) BIOSDATAPAGE);                       /* use processor state in BIOS Data Page */
    unsigned int excCode = CAUSE_GET_EXCCODE(BIOSDPState->cause);   /* retrieve the exceptionCode */

    switch(excCode){
        case EXC_INT:                /* Interrupt */
            interruptHandler();
            break;
        case EXC_MOD ... EXC_TLBS:   /* TLB exceptions */
            passUporDie(PGFAULTEXCEPT);
            break;
        case EXC_SYS:                /* Syscall */
            syscallExcHandler();
            break;
        default:                     /* Program Traps */
            passUporDie(GENERALEXCEPT);    
            break;
    }
}

void syscallExcHandler(){
    BIOSDPState->pc_epc +=WORDLEN;                              /* increase PC to avoid loop on Syscall */
    if (((getSTATUS() & STATUS_KUp)>>STATUS_KUp_BIT) == ON){    /* check if process is in Kernel Mode */

        BIOSDPState->cause = ((EXC_RI << CAUSE_EXCCODE_BIT));   /* set ExcCode to RI */
        passUporDie(GENERALEXCEPT);                             /* call Program Trap Handler */
    }
    else {
        switch (BIOSDPState->reg_a0) {                          /* switch on value in a0 register */

          case CREATEPROCESS:
              BIOSDPState->reg_v0 = (int) createProcess(
                  (state_t*)(BIOSDPState->reg_a1),
                  (support_t*)(BIOSDPState->reg_a2),
                  (nsd_t*)(BIOSDPState->reg_a3));
              break;

          case TERMPROCESS:
              terminateProcess((int)BIOSDPState->reg_a1);
              break;

          case PASSEREN:
              if (Passeren((int *)BIOSDPState->reg_a1)){
                  SAVESTATE;
                  scheduler();
              }
              break;

          case VERHOGEN:
              if (Verhogen((int *)BIOSDPState->reg_a1)){
                  SAVESTATE;
                  scheduler();
              }
              break;

          case DOIO:
              BIOSDPState->reg_v0 = (int) DO_IO(
                  (int *)(BIOSDPState->reg_a1),
                  (int *)(BIOSDPState->reg_a2));
                  SAVESTATE;
                  scheduler();
              break;

          case GETTIME:
              BIOSDPState->reg_v0 = (int) getTime();
              break;

          case CLOCKWAIT:
              waitForClock();
              SAVESTATE;
              scheduler();
              break;

          case GETSUPPORTPTR:
              BIOSDPState->reg_v0 = (int) getSupportData();
              break;

          case GETPROCESSID:
              BIOSDPState->reg_v0 = (int) getProcessID((int)BIOSDPState->reg_a1);
              break;

          case GETCHILDREN:
              BIOSDPState->reg_v0 = (int) getChildren(
                  (int*)(BIOSDPState->reg_a1),
                  (int)(BIOSDPState->reg_a2));
              break;

          default: /* Syscall 11 and above */
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
    if (newProc == NULL) /* no PCB to assign */
        return 0;
    PID++;
    processCount++;

    /* newProc variables initialization */
    newProc->p_s = (*statep);  
    newProc->p_supportStruct = supportp;
    newProc->p_time = 0;
    newProc->p_semAdd = NULL;
    newProc->p_pid = PID;

    /* new process is set as currentProcess' child and added to the readyQueue */
    insertChild(currentProcess, newProc);
    insertProcQ(&readyQueue, newProc);

    /* if the ns passed is NULL, it fails and enters */
    if(!addNamespace(newProc, ns)){
        for(int i=0; i<NS_TYPE_MAX; i++){
            /* gives the new process the parent's namespace */
            nsd_t* tmpNs = getNamespace(currentProcess, i);
            addNamespace(newProc, tmpNs);
        }
    }
    return PID;
}

void terminateProcess(int pid){
    processCount--;                                                                   
    pcb_t* proc = (pid==0||pid==currentProcess->p_pid)?currentProcess:findPCB_pid(pid,(&readyQueue));
    outChild(proc);                                                                             /* the process is separated from its parent */    
    if(proc->p_semAdd!=NULL){                                                                   /* Case 1: the process is blocked on a semaphore */
        int * tmpSem = proc->p_semAdd;                                                          
        outBlocked(proc);                                                       
        if((deviceSem <= tmpSem && tmpSem <= deviceSem + ALDEV) || tmpSem == &pseudoClockSem){  /* softBlock Semaphore*/
            SBcount--;                                                                             
        }
    }else if (proc!=currentProcess){                                                            /* Case 2: process in readyQueue */
        outProcQ(&readyQueue, proc);                                                            
    }
    while(!emptyChild(proc)){                                                                   /* recursively terminate every child (and its subtree) */
        pcb_t* firstChild = list_first_entry(&proc->p_child,struct pcb_t,p_child);          
        removeChild(proc);                                                                      /* remove the first child and eventually move the next one in its place */
        terminateProcess(firstChild->p_pid);                                                
    }
    freePcb(proc);                                                                              /* finally delete the process */
    if(currentProcess==proc){                                                                   /* no process to return control to */
        scheduler();
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


int DO_IO(int *cmdAddr, int *cmdValues){
    currentProcess->valueAddr = (unsigned int *)cmdValues;

    /* find the correct device for I/O from its address */
    int line   = findLine(cmdAddr);                                
    int device = findDevice(line,cmdAddr);                      
    int indexDevice = (IDEVCLASS(line)) + device;

    if (indexDevice < 0){                           /* error case */
        return -1;
    }
    else if (indexDevice < (DEVICECNT-DEVPERINT)){  /* non-terminal device */
        dtpreg_t* dev = (dtpreg_t*)(cmdAddr);
        dev->command = cmdValues[COMMAND];
        dev->data0 = cmdValues[DATA0];
        dev->data1 = cmdValues[DATA1];
    }
    else {                                          /* terminal device */
        termreg_t* terminal;
        if (indexDevice%2==0){      /* recv */
            terminal = (termreg_t*)cmdAddr;
            terminal->recv_command = cmdValues[COMMAND]; 
        }
        else {                      /* transm */
            terminal = (termreg_t*)((int)cmdAddr - (DEV_REG_SIZE/TERMSUB)); /* cmdAddr is the trasm address, so the terminal is cmdAddress - 8*/
            terminal -> transm_command = cmdValues[COMMAND];
        } 
    }
    /* block the process on that device */
    int *sem = &deviceSem[indexDevice];
    P(sem);

    return 0;
}


int findLine(int *cmdAddr){
    return (NOTDEV(cmdAddr)) ? NOPROC : LINEDEV(cmdAddr); 
}


int findDevice(int line,int* cmdAddr){
    int devNum = (line==IL_TERMINAL ? DEVPERINT : DEVPERINT*TERMSUB); 
    return (int)((int)cmdAddr - (int)DEV_REG_ADDR(line,0)) / devNum;
}


cpu_t getTime(){
    return (currentProcess->p_time);
}


void waitForClock(){
    if(pseudoClockSem == 0){
        insertBlocked(&pseudoClockSem, currentProcess);
        SBcount++;
    }
}


support_t* getSupportData(){
    return currentProcess->p_supportStruct;
}


int getProcessID(int parent){
    if (parent){
        nsd_t* ns = getNamespace(currentProcess, NS_PID);
        if (ns == getNamespace(currentProcess->p_parent,NS_PID)) /*checks if they belong to the same namespace */
            return currentProcess-> p_parent-> p_pid;

        return 0;
    }
    else return currentProcess->p_pid;
}


int getChildren(int* children, int size){
    int valueToReturn = 0;                                                                      
    if (!emptyChild(currentProcess)){                                                           /* check if pcb has children*/
        pcb_t* firstChild = list_first_entry(&currentProcess->p_child,struct pcb_t,p_child);    
        nsd_t* currentNs = getNamespace(currentProcess, NS_PID);                               
        if (currentNs == getNamespace(firstChild, NS_PID)){                                    
            if (valueToReturn < size)                                                           /* insert the first child if possible */
                *(children) = firstChild->p_pid; 
            valueToReturn ++;                                                                    
        }

        struct pcb_t* iterator = NULL;
        list_for_each_entry(iterator,&firstChild->p_sib,p_sib){                                 /* check for every other children */
            if (currentNs == getNamespace(iterator, NS_PID)){   
                if (valueToReturn < size)
                    *(children + valueToReturn) = iterator->p_pid;                              /* contiguously insert the other children if possible */ 
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
