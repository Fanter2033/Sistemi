#include "pcb.h"
#include "ns.h"
#include "ash.h"
#include <pandos_const.h>
#include <pandos_types.h>
#include <umps3/umps/cp0.h>
#include <umps3/umps/libumps.h>
#include <umps3/umps/arch.h>

/* take T and N and return the n-th bit of T */

#define DEVNUM 8
/* find nth bit of n */
#define NBIT(T,N) ((T & (1 << N)) >> N) 
/*CauseIP part of Cause register*/
#define CAUSEIP ((getCAUSE() & CAUSE_IP_MASK) >> CAUSE_IP_BIT(0)) 
#define DISABLEINT(LINE) setCAUSE(getCAUSE() & (~((1<<LINE)<<CAUSE_IP_BIT(0))));

#define TERMSTATMASK 0xFF

extern int readyPCB;

extern int findDevice(int line,int* cmdAddr);
extern int processCount;
extern int SBcount;
//extern int deviceSem[ALDEV]; 
extern struct list_head readyQueue;
extern pcb_t* currentProcess;
extern void schedule();
extern state_t* BIOSDPState;
extern void waitForClock();
extern int pseudoClockSem;

extern int diskSem[DEVNUM];
extern int flashSem[DEVNUM];
extern int netSem[DEVNUM];
extern int printerSem[DEVNUM];
extern int termSem[(DEVNUM*2)];

void P(int* sem);
pcb_t* V(int* sem);
void nonTimerInterruptHandler(int interruptLine);
void resolveTerm(int line, int device);
void resolveNonTerm(int line, int device);
void ITInterrupt();
void PLTinterrupt();

int indexDevice;

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
            /* n-th bit line resolved, so 1->0*/
            DISABLEINT(line);
        }
    }
    /* return control to Current Process */
    if (currentProcess==NULL){
        schedule();
    }
    LDST(BIOSDPState);
}


void nonTimerInterruptHandler(int interruptLine){
    /* to store the Bit Map of line */
    int* deviceBitMap;
    int nBit=0;

    switch (interruptLine){
        
        case IL_DISK:
            nBit = 0;
            deviceBitMap = (int *)CDEV_BITMAP_ADDR(IL_DISK);
            for (int bit=0;bit<8;bit++){
                if (NBIT(*deviceBitMap,bit)==ON){
                    resolveTerm(interruptLine,bit);     //NON TERM , da cambiare
                    *deviceBitMap &= (~(1<<bit));
                }
            }
            break;

        case IL_FLASH:
            nBit = 0;
            deviceBitMap = (int *)CDEV_BITMAP_ADDR(IL_FLASH);
            for (int bit=0;bit<8;bit++){
                if (NBIT(*deviceBitMap,bit)==ON){
                    resolveTerm(interruptLine,bit);
                    *deviceBitMap &= (~(1<<bit));
                }
            }
            break;
        
        case IL_ETHERNET:
            nBit = 0;
            deviceBitMap = (int *)CDEV_BITMAP_ADDR(IL_ETHERNET);
            for (int bit=0;bit<8;bit++){
                if (NBIT(*deviceBitMap,bit)==ON){
                    resolveTerm(interruptLine,bit);
                    *deviceBitMap &= (~(1<<bit));
                }
            }
            break;
        
        case IL_PRINTER:
            nBit = 0;
            deviceBitMap = (int *)CDEV_BITMAP_ADDR(IL_PRINTER);
            for (int bit=0;bit<8;bit++){
                if (NBIT(*deviceBitMap,bit)==ON){
                    resolveTerm(interruptLine,bit);
                    *deviceBitMap &= (~(1<<bit));
                }
            }
            break;
        
        case IL_TERMINAL:
            nBit = 0;
            deviceBitMap = (int *)CDEV_BITMAP_ADDR(IL_TERMINAL);
            for (int bit=0;bit<8;bit++){
                if (NBIT(*deviceBitMap,bit) == ON){
                    resolveTerm(interruptLine,bit);
                    *deviceBitMap &= (~(1<<bit));
                }
            }
            break;
    }
}

void resolveTerm(int line, int device){
    termreg_t* termReg = (DEV_REG_ADDR( line, device));
    int* sem;
    indexDevice=-1;     //reinitialize indexDevice
    
    if( termReg->transm_status > 1 && termReg->transm_status != BUSY ) {
        unsigned int status = (termReg->transm_status) & TERMSTATMASK;
        termReg->transm_command = ACK ; 
        indexDevice = findDevice(IL_TERMINAL,((int)termReg)+8);
        sem = &(termSem[indexDevice]);
        /* V on trasm (sub) device */
        pcb_t* unlockedPCB = V(sem);
        if (unlockedPCB != NULL){
            unlockedPCB->p_s.reg_v0 = 0;    //DOIO è andata a buon fine
            (unlockedPCB->valueAddr)[0] = status;
            /* insert unlocked in ready queue*/
            insertProcQ(&readyQueue,unlockedPCB);
        }
    }
    indexDevice = -1;
    if(termReg->recv_status >1 && termReg->recv_status != BUSY){
        unsigned int status = (termReg->recv_status)& TERMSTATMASK;
        termReg->recv_command = ACK;
        indexDevice = findDevice(IL_TERMINAL,((int)termReg));
        sem = (&termSem[indexDevice]);
        /* V on recv (sub) device */
        pcb_t* unlockedPCB = V(sem);
        if (unlockedPCB != NULL){
            unlockedPCB->p_s.reg_v0 = 0;        //DOIO è andata a buon fine
            (unlockedPCB->valueAddr)[0] = status;
            /* insert unlocked in ready queue*/
            insertProcQ(&readyQueue,unlockedPCB);
        }
    }  
}

void resolveNonTerm(int line, int device){

    /* take device register from Address */
    dtpreg_t* devReg = (DEV_REG_ADDR( line, device));
    
    /* save off the status from device register */
    unsigned int status = devReg -> status;
    /* ACK the interrupt */
    devReg->command = ACK;

    /* V on semaphore */
    int* sem = printerSem[device]; // vedi operazione in DOIO, è identica
    pcb_t* waitingPCB = headBlocked(sem);
    if (waitingPCB != NULL){
        /* unlock PCB */
        V(sem);
        waitingPCB ->p_s.reg_v0 = 0;
        (waitingPCB->valueAddr)[0] = status;
        /* insert in readyQueue */
        insertProcQ(&readyQueue,waitingPCB);
        readyPCB++;
    }
    
}


/* P on device semaphore */
void P(int* sem){

    if ((*sem) <= 0 ){
        insertBlocked(sem, currentProcess);
        SBcount++;
    }
    else
        *sem = 0;

}

/* V on device semaphore */
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
        DISABLEINT(1);
        setTIMER(50);
        currentProcess->p_s = (*BIOSDPState);
        insertProcQ(&readyQueue,currentProcess);
        schedule();
    }
}

void ITInterrupt(){ //ogni 100 millisecondi
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