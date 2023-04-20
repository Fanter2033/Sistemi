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

extern int findDevice(int* cmdAddr);
extern int processCount;
extern int SBcount;
extern int deviceSem[ALDEV]; 
extern struct list_head readyQueue;
extern pcb_t* currentProcess;
extern void schedule();
extern state_t* BIOSDPState;

void P(int* sem);
void V(int* sem);


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
            case IL_CPUTIMER: // 1
                //3.6.2
                break;
            
            case IL_TIMER:
                //3.6.3
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

void nonTimerInterruptHandler(int interruptLine){
    /* to store the Bit Map of line */
    int deviceBitMap;
    int nBit=0;

    switch (interruptLine){
        case IL_DISK:
            nBit = 0; /* iterator on all bit of integer value */
            deviceBitMap = CDEV_BITMAP_ADDR(IL_DISK);
        
            while(deviceBitMap != 0){
                if (NBIT(deviceBitMap,nBit) == ON){
                    handleInterrupt(interruptLine,nBit);
                    deviceBitMap &= (0<<nBit);
                }
                nBit++;
            }
            break;

        case IL_FLASH:
            nBit = 0;
            deviceBitMap = CDEV_BITMAP_ADDR(IL_FLASH);

            while(deviceBitMap != 0){
                if (NBIT(deviceBitMap,nBit) == ON){
                    handleInterrupt(interruptLine,nBit);
                    deviceBitMap &= (0<<nBit);
                }
                nBit++; 
            }
            break;
        
        case IL_ETHERNET:
            nBit = 0;
            deviceBitMap = CDEV_BITMAP_ADDR(IL_ETHERNET);

            while(deviceBitMap != 0){
                if (NBIT(deviceBitMap,nBit) == ON){
                    handleInterrupt(interruptLine,nBit);
                    deviceBitMap &= (0<<nBit);
                }
                nBit++; 
            }
            break;
        
        case IL_PRINTER:
            nBit = 0;
            deviceBitMap = CDEV_BITMAP_ADDR(IL_PRINTER);

            while(deviceBitMap != 0){
                if (NBIT(deviceBitMap,nBit) == ON){
                    handleInterrupt(interruptLine,nBit);
                    deviceBitMap &= (0<<nBit);
                }
                nBit++;
            }
            break;
        
        case IL_TERMINAL:
            nBit = 0;
            deviceBitMap = CDEV_BITMAP_ADDR(TERMINT);

            while(deviceBitMap !=0 ){
                if(NBIT(deviceBitMap,nBit) == ON){
                    handleInterrupt(interruptLine,nBit);
    
                }
            }
            //capire come gestire questo degli interrupt
            break;
        
        default:
            break;
    }
}

void terminalInterrupt(int line, int device){
    termreg_t* termReg = ((termreg_t *)DEV_REG_ADDR( line, device));

    if(NBIT(){
        unsigned int status = termReg->transm_status;
        termReg->transm_command = 1 << 1 ; 

    }
    if(){
        unsigned int status = termReg->recv_status;
        termReg->recv_command = 1 << 1;
    }
    
    
}
/

void handleInterrupt(int line, int device){

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

/* P on device semaphore */
void P(int* sem){       //VEDERE SE SI PUÒ GENERALIZZARE LA P DELLE SYS
    if (*sem ==0 ){
        insertBlocked(sem, currentProcess);
        SBcount++;
    }
    else if (headBlocked(sem)!=NULL){
        insertBlocked(sem,currentProcess);
        insertProcQ(&readyQueue,removeBlocked(sem));
        SBcount++;
    }
    else{
        // in realtà questo è un caso in cui non arriva mai visto che sono tutti 
        //semafori di sincronizzazione, ergo partono da 0
        sem--;
        SBcount++;
    }
}

/* V on device semaphore */
void V(int* sem){       //VEDERE SE SI PUÒ GENERALIZZARE LA V DELLE SYS
/* 
    credo si possa migliorare, non andrà mai a 1 visto che sono di sincronizzazione 
    appena ci arriva in realtà ripassa subito a 0
*/
    if (*sem ==1 ){
        insertBlocked(sem, currentProcess);
        SBcount++;
    }
    else if (headBlocked(sem)!=NULL){
        insertBlocked(sem,currentProcess);
        insertProcQ(&readyQueue,removeBlocked(sem));
        SBcount++;
    }
    else{
        sem++;
        SBcount--;
    }
}