#include "ns.h"
#include "ash.h"
#include <umps3/umps/libumps.h>
#include <umps3/umps/cp0.h>

#define ALDEV 50  /* Number of (sub)devices semaphores */

/*
    All Lines Devices:

    To be reworked to:
    0...7 -> Disk Devices
    8...15 -> Flash Devices
    16...23 -> Network Devices
    24...31 -> Printer Devices
    32...47 -> Terminal Devices:
        32 33 Terminal 0 Recv, Trasm
        34 35 Terminal 1 R,T
        ...
        46 47 Terminal 7 R,T
*/

/* ---- Extern Functions ---- */

extern void test();                 
extern void uTLB_RefillHandler();
extern void exceptionHandler();
extern void schedule();

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