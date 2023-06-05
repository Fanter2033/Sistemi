#include <pcb.h>
#include <umps3/umps/libumps.h>
#include <umps3/umps/cp0.h>

/* ---- Extern Variables ---- */

extern int processCount;
extern int SBcount;
extern struct list_head readyQueue;
extern pcb_t* currentProcess;
extern int processStartTime;

/* ---- Functions Declaration ---- */

void schedule();