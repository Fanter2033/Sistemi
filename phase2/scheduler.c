#include "pcb.h"
#include "ns.h"
#include "ash.h"
#include <pandos_const.h>
#include <pandos_types.h>
#include <umps3/umps/libumps.h>

extern int processCount;
extern int SBcount;
extern struct list_head* readyQueue;
extern pcb_t* currentProcess;

void schedule(){
    if (emptyProcQ(readyQueue)){
        if (processCount==0) 
            HALT();        /* nothing else to do */
        else if (processCount > 0 && SBcount > 0) {

            /* interrupts enabled, PLT disabled */
                /* KU ( Kernel Mode ) on ?? */
            setSTATUS( STATUS_IEp | STATUS_KUp | STATUS_IM_MASK & (~STATUS_TE) );    
            WAIT();     /* waiting for I/O interrupt */

            /*
                Per settare il bit del PLT a 0, devo fare un & con una sequenza di 1 e uno 0 
                nella posizione che voglio annullare

                001000100010 & 
                000111111111 = 
                000000100010
            */
        }
        else if (processCount > 0 && SBcount == 0) 
            PANIC();     /* deadlock found */
    }
    currentProcess = removeProcQ(readyQueue);
    setTIMER(5);
    LDST(currentProcess->p_s);
}