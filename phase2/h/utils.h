#ifndef UTILS_H
#define UTILS_H

#include <ns.h>
#include <ash.h>
#include <umps3/umps/cp0.h>
#include <umps3/umps/arch.h>
#include <umps3/umps/libumps.h>
#include <umps3/umps/types.h>

#define TERMSUB 2                   /* Sub device for terminal */
#define ALDEV DEVICECNT + DEVINTNUM   /* Number of (sub)devices semaphores */
#define SAVESTATE currentProcess->p_s = *BIOSDPState


/*
    All Lines Devices [ALDEV] represents:
    0...7 -> Disk Devices
    8...15 -> Flash Devices
    16...23 -> Network Devices
    24...31 -> Printer Devices
    32...47 -> Terminal Devices:
        32 33 Terminal 0 Recv, Trasm
        ...
        46 47 Terminal 7 R,T
*/


/* ---- Extern Variables ---- */

extern int processCount;
extern int SBcount;
extern int deviceSem[ALDEV]; 
extern struct list_head readyQueue;
extern pcb_t* currentProcess;
extern int pseudoClockSem;
extern state_t* BIOSDPState;
extern int processStartTime;
extern int PID;

/* ---- Extern Functions ---- */

extern void test();                 
extern void uTLB_RefillHandler();
extern void exceptionHandler();
extern void scheduler();
extern void interruptHandler();
extern void P(int* sem);

#endif