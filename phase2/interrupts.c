#include "interrupts.h"

void interruptHandler(){
    /* line 0 not considered (NO multiprocessor) */
    /* check goes from most significant bit to lower, so the priority is considered */
    for (int line=1; line<8;line++){
        if (NBIT(CAUSEIP,line) == ON){     //INTERRUPT LINE ON
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
    /* to store the Bit Map of line */
    int* deviceBitMap = (int *)CDEV_BITMAP_ADDR(interruptLine);
    int nBit=0;
    for (int bit=0;bit<DEVPERINT;bit++){
        if (NBIT(*deviceBitMap,bit)==ON){
            if (interruptLine==IL_TERMINAL){
                resolveTerm(interruptLine,bit);
            } else{
                resolveNonTerm(interruptLine,bit);
            }
            *deviceBitMap &= (~(1<<bit));
        }
    }
}


void resolveTerm(int line, int device){
    termreg_t* termReg = (DEV_REG_ADDR( line, device));
    int sem;
    int indexDevice;
    
    if( termReg->transm_status >1 && termReg->transm_status != BUSY ) {
        unsigned int status = (termReg->transm_status) & TERMSTATMASK;
        termReg->transm_command = ACK ;
        indexDevice = ((line-3)*8)+2+ (2*device) + 1 ;
        sem = &deviceSem[indexDevice];
        /* V on trasm (sub) device */
        pcb_t* unlockedPCB = V((int*)sem);
        if (unlockedPCB != NULL){
            unlockedPCB->p_s.reg_v0 = 0;    //DOIO è andata a buon fine
            (unlockedPCB->valueAddr)[0] = status;
            /* insert unlocked in ready queue*/
            insertProcQ(&readyQueue,unlockedPCB);
        }
    }

    if(termReg->recv_status >1 && termReg->recv_status != BUSY){
        unsigned int status = (termReg->recv_status)& TERMSTATMASK;
        termReg->recv_command = ACK;
        indexDevice = ((line-3)*8)+2+ (2*device);
        sem = &deviceSem[indexDevice];
        /* V on recv (sub) device */
        pcb_t* unlockedPCB = V((int*)sem);
        if (unlockedPCB != NULL){
            unlockedPCB->p_s.reg_v0 = 0;    //DOIO è andata a buon fine
            (unlockedPCB->valueAddr)[0] = status;
            /* insert unlocked in ready queue*/
            insertProcQ(&readyQueue,unlockedPCB);
        }
    }  
}

void resolveNonTerm(int line, int device){

    /* take device register from Address */
    dtpreg_t* devReg = (DEV_REG_ADDR( line, device));
    int sem;
    int indexDevice;

    if(devReg->status > 1 && devReg->status != BUSY){
        /* save off the status from device register */
        unsigned int status = devReg -> status;
        /* ACK the interrupt */
        devReg->command = ACK;
        indexDevice = ((line-3)*8)+2+device;
        sem = &deviceSem[indexDevice];
        /* V on semaphore */
        pcb_t* unlockedPCB = V((int*)sem);
        if (unlockedPCB != NULL){
            unlockedPCB->p_s.reg_v0 = 0;    //DOIO a buon fine
            (unlockedPCB->valueAddr)[0] = status;
            /* insert unlocked in readyQueue */
            insertProcQ(&readyQueue,unlockedPCB);
        }
    }
    
}

void P(int* sem){
    if (*sem <=0 ){
        insertBlocked(sem, currentProcess);
        SBcount++;
    }
    else
        *sem = 0;

}

pcb_t* V(int* sem){
    if (headBlocked(sem) == NULL){
        (*sem) = 1;
        return NULL;
    }
    else{
        pcb_t* unlocked = removeBlocked(sem);
        SBcount--;
        return unlocked;
    }
}

void PLTinterrupt(){
    
    if(((getSTATUS() & STATUS_TE) >> STATUS_TE_BIT) == ON){
        currentProcess->p_s = (*BIOSDPState);
        insertProcQ(&readyQueue,currentProcess);
        schedule();
    }
}

void ITInterrupt(){ /* it happens every 100 millisecond */
    /* ack the interrupt */
    LDIT(PSECOND);
    /* unlock ALL processes */
    while(headBlocked(&pseudoClockSem)!=NULL){
        insertProcQ(&readyQueue,removeBlocked(&pseudoClockSem));
        SBcount--;
    }   
    /* set pseudoClockSem to 0 */
    pseudoClockSem = 0;
    if (currentProcess==NULL){
        schedule();
    }
    LDST(BIOSDPState);
}