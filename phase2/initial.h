#ifndef INITIAL_H
#define INITIAL_H

#include <utils.h>

/* ---- Global Variables ---- */

int processCount;               /* processes started but not yet terminated */
int SBcount;                    /* soft-blocked processes (blocked on deviceSem or pseudoClockSemaphore) */
pcb_t* currentProcess;          /* pcb that is in running state */
struct list_head readyQueue;    /* queue of ready pcb */
int deviceSem[ALDEV];           /* array of device semaphores */

int pseudoClockSem;             /* semaphore for Interval Timer purposes */
int processStartTime;           /* currentProcess' time of CPU usage */
int PID = 1;                    /* for keeping track of processes PID */

HIDDEN passupvector_t* passUpCP0; /* passupvector for CP0 */

/* ---- Functions Declaration ---- */

int main();

#endif