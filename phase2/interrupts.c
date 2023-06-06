#include "interrupts.h"

void interruptHandler(){                    /* Line 0 has not to be considered (No multi processor) */
    for (int line=1; line<N_IL;line++){     /* Check from most significant bit (priority considered)*/
        if (NBIT(CAUSEIP,line) == ON){      /* Check if the interrupt line is on */
            switch (line){
                case IL_CPUTIMER:
                    PLTinterrupt();
                    break;

                case IL_TIMER:
                    ITInterrupt();
                    break;

                default:                    
                    nonTimerInterruptHandler(line);
                    break;
            }
        }
    }
    /* return control to Current Process if it exists*/
    if (currentProcess==NULL){
        schedule();
    }
    LDST(BIOSDPState);
}


void nonTimerInterruptHandler(int interruptLine){
    int* deviceBitMap = (int *)CDEV_BITMAP_ADDR(interruptLine);     /* store the Bit Map of line */
    for (int bit=0;bit<DEVPERINT;bit++){                            /* check for all the devices of the line*/
        if (NBIT(*deviceBitMap,bit)==ON){                           /* check if the device has an interrupt pending */
            if (interruptLine==IL_TERMINAL){
                resolveTerm(interruptLine,bit);
            } else{
                resolveNonTerm(interruptLine,bit);
            }
            *deviceBitMap &= (~(1<<bit));                           /* set the corresponding bit to 0 */
        }
    }
}


void resolveTerm(int line, int device){
    termreg_t* termReg = (termreg_t*)(DEV_REG_ADDR( line, device));
    int* sem;
    int indexDevice;
    
    if( termReg->transm_status > READY && termReg->transm_status != BUSY ) {
        unsigned int status = (termReg->transm_status) & TERMSTATMASK;
        termReg->transm_command = ACK ;
        indexDevice = (EXT_IL_INDEX(line)*DEVPERINT)+(2*device) + 1 ;
        sem = &deviceSem[indexDevice];
        /* V on trasm (sub) device */
        pcb_t* unlockedPCB = V(sem);
        if (unlockedPCB != NULL){
            /* DO_IO ended correctly */
            unlockedPCB->p_s.reg_v0 = 0;
            (unlockedPCB->valueAddr)[0] = status;
            /* insert unlocked in ready queue*/
            insertProcQ(&readyQueue,unlockedPCB);
        }
    }

    if(termReg->recv_status > READY && termReg->recv_status != BUSY){
        unsigned int status = (termReg->recv_status)& TERMSTATMASK;
        termReg->recv_command = ACK;
        indexDevice = (EXT_IL_INDEX(line)*DEVPERINT)+(2*device);
        sem = &deviceSem[indexDevice];
        /* V on recv (sub) device */
        pcb_t* unlockedPCB = V(sem);
        if (unlockedPCB != NULL){
            /* DO_IO ended correctly */
            unlockedPCB->p_s.reg_v0 = 0;
            (unlockedPCB->valueAddr)[0] = status;
            /* insert unlocked in ready queue*/
            insertProcQ(&readyQueue,unlockedPCB);
        }
    }  
}

void resolveNonTerm(int line, int device){

    /* take device register from Address */
    dtpreg_t* devReg = (dtpreg_t*)(DEV_REG_ADDR( line, device));
    int* sem;
    int indexDevice;

    if(devReg->status > READY && devReg->status != BUSY){
        /* save off the status from device register */
        unsigned int status = devReg -> status;
        /* ACK the interrupt */
        devReg->command = ACK;
        indexDevice = ((EXT_IL_INDEX(line))*DEVPERINT)+ device;
        sem = &deviceSem[indexDevice];
        /* V on semaphore */
        pcb_t* unlockedPCB = V(sem);
        if (unlockedPCB != NULL){
            /* DO_IO ended correctly */
            unlockedPCB->p_s.reg_v0 = 0;
            (unlockedPCB->valueAddr)[0] = status;
            /* insert unlocked in readyQueue */
            insertProcQ(&readyQueue,unlockedPCB);
        }
    }
    
}

void P(int* sem){
    if (*sem <=0 ){ /* blocking */
        insertBlocked(sem, currentProcess);
        SBcount++;
    }
    else
        *sem = 0;

}

pcb_t* V(int* sem){
    if (headBlocked(sem) == NULL){ /* no pcb in queue */
        (*sem) = 1;
        return NULL;
    }
    else{
        pcb_t* unlocked = removeBlocked(sem); /* unblocking */
        SBcount--;
        return unlocked;
    }
}

void PLTinterrupt(){
    if(((getSTATUS() & STATUS_TE) >> STATUS_TE_BIT) == ON){ /* check if the local timer is enabled */
        setTIMER(TIMESLICE);                                /* ack the interrupt */
        currentProcess->p_s = (*BIOSDPState);               /* save the processor state */
        insertProcQ(&readyQueue,currentProcess);            /* pcb transitioned to the ready state */
        schedule();
    }
}

void ITInterrupt(){
    LDIT(PSECOND);                                          /* ack the interrupt */
    while(headBlocked(&pseudoClockSem)!=NULL){              /* unlock ALL processes */
        insertProcQ(&readyQueue,removeBlocked(&pseudoClockSem));
        SBcount--;
    }   
    pseudoClockSem = 0;                                     /* set pseudoClockSem to 0 */
}