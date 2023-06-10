#include "scheduler.h"

void scheduler(){
    currentProcess = NULL;
    if (emptyProcQ(&readyQueue)){
        if (processCount==0)                         /* case 1: nothing else to do */
            HALT();                                 
        else if (processCount > 0 && SBcount > 0) {  /* case 2: waiting for some I/O interrupt */
            /* interrupts enabled, PLT disabled */  
            setSTATUS((IECON | IMON) & (~TEBITON) );
            WAIT();                           
        }
        else if (processCount > 0 && SBcount == 0)  /* case 3: deadlock found */
            PANIC();
    }
    currentProcess = removeProcQ(&readyQueue);      /* readyQueue is not empty */
    setTIMER(TIMESLICE);                            /* set the processor's local timer */
    STCK(processStartTime);                         /* set the starting time of the currentProcess */
    LDST(&(currentProcess->p_s));                   /* load the current process for execution */
}