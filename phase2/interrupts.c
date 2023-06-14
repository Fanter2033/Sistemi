#include "interrupts.h"

void interruptHandler(){                                /* Line 0 has not to be considered (No multi processor) */
    for (int line=IL_CPUTIMER; line<N_IL;line++){       /* Check from most significant bit (priority considered)*/
        if (NBIT(CAUSEIP,line) == ON){                  /* Check if the interrupt line is on */
            switch (line){
                case IL_CPUTIMER:
                    PLTinterrupt();
                    break;

                case IL_TIMER:
                    ITinterrupt();
                    break;

                default:                    
                    nonTimerInterruptHandler(line);
                    break;
            }
        }
    }
    /* return control to Current Process if it exists*/
    if (currentProcess==NULL){
        scheduler();
    }
    LDST(BIOSDPState);
}


void nonTimerInterruptHandler(int interruptLine){
    int* deviceBitMap = (int *)CDEV_BITMAP_ADDR(interruptLine);     /* store the Bit Map of line */
    for (int bit=0; bit < DEVPERINT; bit++){                        /* check for all the devices of the line*/
        if (NBIT(*deviceBitMap,bit) == ON){                         /* check if the device has an interrupt pending */
            if (interruptLine == IL_TERMINAL)
                resolveTerm(interruptLine,bit);
            else
                resolveNonTerm(interruptLine,bit);
            
            *deviceBitMap &= (~(1<<bit));                           /* set the corresponding bit to 0 */
        }
    }
}

void unlockPCB(int index, unsigned int status){
    int* sem = &deviceSem[index];
    pcb_t* unlockedPCB = V(sem);                    /* V on (sub) device */
    if (unlockedPCB != NULL){                       /* DO_IO ended correctly */
        unlockedPCB->p_s.reg_v0 = 0;
        (unlockedPCB->valueAddr)[STATUS] = status; 
        insertProcQ(&readyQueue,unlockedPCB);       /* Insert unlocked in readyQueue */
    }
}

void resolveTerm(int line, int device){
    termreg_t* termReg = (termreg_t*)(DEV_REG_ADDR( line, device));

    int indexDevice;
    
    if(termReg->transm_status > READY && termReg->transm_status != BUSY){
        unsigned int status = (termReg->transm_status) & TERMSTATMASK;           /* save off the status from device register */
        termReg->transm_command = ACK ;                                          /* ACK the interrupt */
        indexDevice = (EXT_IL_INDEX(line)*DEVPERINT)+(TERMSUB*device) + 1;       /* find the correct device */
        unlockPCB(indexDevice, status);
    }

    if(termReg->recv_status > READY && termReg->recv_status != BUSY){
        unsigned int status = (termReg->recv_status)& TERMSTATMASK;
        termReg->recv_command = ACK;
        indexDevice = (EXT_IL_INDEX(line)*DEVPERINT)+(TERMSUB*device);
        unlockPCB(indexDevice, status);
    }  
}

void resolveNonTerm(int line, int device){
    dtpreg_t* devReg = (dtpreg_t*)(DEV_REG_ADDR( line, device));

    if(devReg->status > READY && devReg->status != BUSY){
        unsigned int status = devReg -> status;                     /* save off the status from device register */
        devReg->command = ACK;                                      /* ACK the interrupt */
        int indexDevice = ((EXT_IL_INDEX(line))*DEVPERINT)+ device; /* find the correct device */
        unlockPCB(indexDevice, status);
    }
    
}


void P(int* sem){
    if (*sem <=0 ){                              /* blocking */
        insertBlocked(sem, currentProcess);
        SBcount++;
    }
    else
        *sem = 0;

}

pcb_t* V(int* sem){
    if (headBlocked(sem) == NULL){               /* no pcb in queue */
        (*sem) = 1;
        return NULL;
    }
    else{
        pcb_t* unlocked = removeBlocked(sem);    /* unblocking */
        SBcount--;
        return unlocked;
    }
}


void PLTinterrupt(){
    if(((getSTATUS() & STATUS_TE) >> STATUS_TE_BIT) == ON){ /* check if the local timer is enabled */
        setTIMER(TIMESLICE);                                /* ack the interrupt */
        SAVESTATE;                                          /* save the processor state */
        insertProcQ(&readyQueue,currentProcess);            /* pcb transitioned to the ready state */
        scheduler();
    }
}

void ITinterrupt(){
    LDIT(PSECOND);                                          /* ack the interrupt */
    while(headBlocked(&pseudoClockSem) != NULL){            /* unlock ALL processes */
        insertProcQ(&readyQueue,removeBlocked(&pseudoClockSem));
        SBcount--;
    }   
    pseudoClockSem = 0;                                     /* set pseudoClockSem to 0 */
}