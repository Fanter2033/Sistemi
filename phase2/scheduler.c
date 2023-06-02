#include "pcb.h"
#include "ns.h"
#include "ash.h"
#include <pandos_const.h>
#include <pandos_types.h>
#include <umps3/umps/libumps.h>
#include <umps3/umps/cp0.h>

extern int readyPCB;

extern int processCount;
extern int SBcount; //SoftBlocked
extern struct list_head readyQueue;
extern pcb_t* currentProcess;
#define NBIT(T,N) ((T & (1 << N)) >> N) 

extern int processStartTime;

void schedule(){
    if (emptyProcQ(&readyQueue)){
        if (processCount==0)                         /* case 1: nothing else to do */
            HALT();                          
        else if (processCount > 0 && SBcount > 0) {  /* case 2: waiting for some I/O interrupt */
            /* NO current Process */
            currentProcess=NULL;
            /* interrupts enabled, PLT disabled */  
            setSTATUS( IECON | IMON & (~TEBITON) );
            WAIT();                           
        }
        else if (processCount > 0 && SBcount == 0)  /* case 3: deadlock found */
            PANIC();
    }
    currentProcess = removeProcQ(&readyQueue);       /* readyQueue is not empty */

    setTIMER(TIMESLICE);
    STCK(processStartTime);

    LDST(&(currentProcess->p_s));
}