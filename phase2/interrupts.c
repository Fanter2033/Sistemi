#include "pcb.h"
#include "ns.h"
#include "ash.h"
#include <pandos_const.h>
#include <pandos_types.h>
#include <umps3/umps/cp0.h>
#include <umps3/umps/libumps.h>
#include <umps3/umps/arch.h>

/* take T and N and return the n-th bit of T */
#define ALDEV 42
/* find nth bit of n */
#define NBIT(T,N) ((T & (1 << N)) >> N) 
/* in our semaphores, Term0 W = 34, Term0 R = 42 */
#define OFFSETRWTERM 8 

extern int findDevice(int* cmdAddr);
extern int processCount;
extern int SBcount;
extern int deviceSem[ALDEV]; 
extern struct list_head readyQueue;
extern struct list_head readyQueue;
extern pcb_t* currentProcess;
extern void schedule();
extern state_t* BIOSDPState;
extern void terminateProcess();
extern void waitForClock();

void P(int* sem);
pcb_t* V(int* sem);
void nonTimerInterruptHandler(int interruptLine);
void resolveTerm(int line, int device);
void resolveNonTerm(int line, int device);
void ITInterrupt();
void PLTinterrupt();


void interruptHandler(){
    /* take causeIP part as integer */
    unsigned int causeIP = ((getCAUSE() & CAUSE_IP_MASK) >> CAUSE_IP_BIT(0)); 
    /* line 0 not considered (NO multiprocessor) */
    int line = 1; 
    // se non funge CauseIP vedere se si può considerare come array
    /* check goes from most significant bit to lower, so the priority is considered */
    while (causeIP != 0){   /* loop to resolve all interrupts */
        if (NBIT(causeIP,line) == ON){     //INTERRUPT LINE ON
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
            causeIP &= (0 << line);
        }
        line++;
    }
    /* return control to Current Process */
    LDST(BIOSDPState);
}


void PLTinterrupt(){
    if(((  STATUS_TE_BIT & STATUS_TE) >> STATUS_TE_BIT) == 1){
        
        setTIMER(TIMESLICE); // sus  
        currentProcess->p_s = *BIOSDPState;

        insertProcQ(&readyQueue,currentProcess);

        schedule(); 
    }
}

void nonTimerInterruptHandler(int interruptLine){
    /* to store the Bit Map of line */
    int deviceBitMap;
    int nBit=0;

    switch (interruptLine){
        case IL_DISK:
            nBit = 0; /* iterator on all bit of integer value */
            deviceBitMap = (*((int *)CDEV_BITMAP_ADDR(IL_DISK))) & 0x00000008;
        
            while(deviceBitMap != 0){
                if (NBIT(deviceBitMap,nBit) == ON){
                    resolveNonTerm(interruptLine,nBit);
                    deviceBitMap &= (0<<nBit);
                }
                nBit++;
            }
            break;

        case IL_FLASH:
            nBit = 0;
            deviceBitMap = (*((int *)CDEV_BITMAP_ADDR(IL_FLASH))) & 0x00000008;

            while(deviceBitMap != 0){
                if (NBIT(deviceBitMap,nBit) == ON){
                    resolveNonTerm(interruptLine,nBit);
                    deviceBitMap &= (0<<nBit);
                }
                nBit++; 
            }
            break;
        
        case IL_ETHERNET:
            nBit = 0;
            deviceBitMap = (*((int *)CDEV_BITMAP_ADDR(IL_ETHERNET))) & 0x00000008;

            while(deviceBitMap != 0){
                if (NBIT(deviceBitMap,nBit) == ON){
                    resolveNonTerm(interruptLine,nBit);
                    deviceBitMap &= (0<<nBit);
                }
                nBit++; 
            }
            break;
        
        case IL_PRINTER:
            nBit = 0;
            deviceBitMap =(*((int *)CDEV_BITMAP_ADDR(IL_PRINTER))) & 0x00000008;

            while(deviceBitMap != 0){
                if (NBIT(deviceBitMap,nBit) == ON){
                    resolveNonTerm(interruptLine,nBit);
                    deviceBitMap &= (0<<nBit);
                }
                nBit++;
            }
            break;
        
        case IL_TERMINAL:
            nBit = 0;
            deviceBitMap = (*((int *)CDEV_BITMAP_ADDR(IL_TERMINAL))) & 0x00000008;

            while(deviceBitMap !=0 ){
                if(NBIT(deviceBitMap,nBit) == ON){
                    resolveTerm(interruptLine,nBit);
    
                }
            }
            break;
        
        default:
            break;
    }
}

void resolveTerm(int line, int device){
    termreg_t* termReg = ((termreg_t *)DEV_REG_ADDR( line, device));
    int* sem;

    if(termReg->transm_status != BUSY && termReg->transm_status !=READY) {
        unsigned int status = termReg->transm_status;
        termReg->transm_command = ACK ; 
        sem = deviceSem[findDevice((int)termReg)];
        /* V on trasm (sub) device */
        pcb_t* unlockedPCB = V(sem);
        if (unlockedPCB != NULL){
            unlockedPCB->p_s.reg_v0 = status;
            /* insert unlocked in ready queue*/
            insertProcQ(&readyQueue,unlockedPCB);
        }
    }

    if(termReg->recv_status != BUSY && termReg->recv_status !=READY){
        unsigned int status = termReg->recv_status;
        termReg->recv_command = ACK;
        sem = deviceSem[findDevice((int)termReg)+OFFSETRWTERM];
        /* V on recv (sub) device */
        pcb_t* unlockedPCB = V(sem);
        if (unlockedPCB != NULL){
            unlockedPCB->p_s.reg_v0 = status;
            /* insert unlocked in ready queue*/
            insertProcQ(&readyQueue,unlockedPCB);
        }
    }   
}

void resolveNonTerm(int line, int device){

    /* take device register from Address */
    dtpreg_t* devReg = ((dtpreg_t *)DEV_REG_ADDR( line, device));
    
    /* save off the status from device register */
    unsigned int status = devReg -> status;
    /* ACK the interrupt */
    devReg->command = ACK;

    /* V on semaphore */
    int* sem = deviceSem[findDevice((int)devReg)];     //Da controllare se è giusto
    pcb_t* waitingPCB = removeBlocked(sem); // SUS, vedi se va rimosso dopo usando la P
    if (waitingPCB != NULL){
        /* unlock PCB */
        V(sem);
        waitingPCB ->p_s.reg_v0 = status;
        /* insert in readyQueue */
        insertProcQ(&readyQueue,waitingPCB);
    }
    
}

/*
    SUS valori del semaforo non utilizzati.Note:
    1. in P ha senso chiamare currentProcess (P chiamata in DOIO dal kernel).
    2. in V non ha senso chiamare current (sarebbe totalmente un altro processo, qui siamo a livello kernel)
    3. P sempre bloccante 3 non decrementa mai
    4. V non è mai bloccante !!! (sus ma ha anche molto senso per i device (sincronizzazione))
    5. valore in questo caso usato come conta!!!
*/

/* P on device semaphore */
void P(int* sem){

    if (*sem <=0 ){
        insertBlocked(sem, currentProcess);
        *sem--;
        SBcount++;
    }

}

/* V on device semaphore */
pcb_t* V(int* sem){


    if (headBlocked(sem) == NULL){
        (*sem) = 0;
        return NULL;
    }
    else{
        pcb_t* unlocked = removeBlocked(sem);
        (*sem)++;
        SBcount--;
        return unlocked;
    }
}

void PLTinterrupt(){
    if(((STATUS_TE_BIT & STATUS_TE) >> STATUS_TE_BIT)==1){
        setTIMER(TIMESLICE);
        currentProcess->p_s = *BIOSDPState;
        insertProcQ(&readyQueue,currentProcess);
        schedule();
    }
}

void ITInterrupt(){
    /* ack the interrupt */
    LDIT(PSECOND);
    /* unlock ALL processes */
    while(headBlocked(&pseudoClockSem)!=NULL){
        insertProcQ(&readyQueue,removeBlocked(&pseudoClockSem));
    }   
    /* set pseudoClockSem to 0 */
    pseudoClockSem=0;
    if (currentProcess==NULL){
        schedule();
    }
    LDST(BIOSDPState);
}