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

            /*
                Nello status register: 
                ATTIVARE GLI INTERRUPT (usando i p. Tocca vedere se usare i 'p' o i 'c')
                1111 1111 0000 1100 -> 65292

                DISATTIVARE PLT
                TE = 0
                voglio mettere a 0 soltanto il 27-esimo bit
                ... & (~(1<<27)) 
            */

            setSTATUS( 65292 & (~(1<<27)) );    /* interrupts enabled, PLT disabled */
            WAIT();     /* waiting for I/O interrupt */
        }
        else if (processCount > 0 && SBcount == 0) 
            PANIC();     /* deadlock found */
    }
    currentProcess = removeProcQ(readyQueue);
    setTIMER(5);
    LDST(currentProcess->p_s);
}