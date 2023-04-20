#include "pcb.h"
#include "ns.h"
#include "ash.h"
#include <pandos_const.h>
#include <pandos_types.h>
#include <umps3/umps/libumps.h>
#include <umps3/umps/cp0.h>

extern int processCount;
extern int SBcount; //SoftBlocked
extern struct list_head readyQueue;
extern pcb_t* currentProcess;
#define NBIT(T,N) ((T & (1 << N)) >> N) 

#define MS(NUM) NUM*100 * (*((cpu_t *) TIMESCALEADDR))

void schedule(){
    if (emptyProcQ(&readyQueue)){
        if (processCount==0)                         /* case 1: nothing else to do */
            HALT();                                 
        else if (processCount > 0 && SBcount > 0) {  /* case 2: waiting for some I/O interrupt */

            /* interrupts enabled, PLT disabled */  
            setSTATUS( IEPON | IMON & (~STATUS_TE) );
            WAIT();                           
        }
        else if (processCount > 0 && SBcount == 0)  /* case 3: deadlock found */
            PANIC();     
    }
    currentProcess = removeProcQ(&readyQueue);       /* readyQueue is not empty */
    setTIMER(MS(5));
    addokbuf("prima dell'inizio del primo processo, addios\n");

    // 7.3.1
    LDST(&(currentProcess->p_s));
}