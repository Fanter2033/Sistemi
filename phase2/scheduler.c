#include "pcb.h"
#include "ns.h"
#include "ash.h"
#include <pandos_const.h>
#include <pandos_types.h>
#include <umps3/umps/libumps.h>
#include <umps3/umps/cp0.h>

extern int processCount;
extern int SBcount; //SoftBlocked
extern struct list_head* readyQueue;
extern pcb_t* currentProcess;

void schedule(){
    if (emptyProcQ(readyQueue)){
        if (processCount==0)                         /* case 1: nothing else to do */
            HALT();                                 
        else if (processCount > 0 && SBcount > 0) {  /* case 2: waiting for some I/O interrupt */

            /* interrupts enabled, PLT disabled */  
            setSTATUS( STATUS_IEp | STATUS_KUp | STATUS_IM_MASK & (~STATUS_TE) );    // KU ( Kernel Mode ) on ??
            WAIT();                           
        }
        else if (processCount > 0 && SBcount == 0)  /* case 3: deadlock found */
            PANIC();     
    }
    currentProcess = removeProcQ(readyQueue);
    setTIMER(5);
    LDST(&(currentProcess->p_s));
}