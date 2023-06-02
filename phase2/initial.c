#include "pcb.h"
#include "ns.h"
#include "ash.h"
#include "list.h"
#include <pandos_const.h>
#include <pandos_types.h>
#include <umps3/umps/libumps.h>
#include <umps3/umps/cp0.h>

#define NBIT(T,N) ((T & (1 << N)) >> N) 
#define DEVNUM 8


int readyPCB;

int processCount;   /*process started but not yet terminated */
int SBcount;    /* soft-blocked count */
pcb_t* currentProcess;  /* pcb that is in running state */
struct list_head readyQueue;  /* queue of ready pcb */

int intervalTimerSem;
int diskSem[DEVNUM];
int flashSem[DEVNUM];
int netSem[DEVNUM];
int printerSem[DEVNUM];
int termSem[(DEVNUM*2)];


int pseudoClockSem;

int processStartTime;

HIDDEN passupvector_t* passUpCP0;

extern void test();
extern void uTLB_RefillHandler();
extern void exceptionHandler();
extern void schedule();

int main(){

    /* passup vector initialization */
    passUpCP0 = (passupvector_t*) PASSUPVECTOR;
    passUpCP0->tlb_refill_handler = (memaddr) uTLB_RefillHandler;
    passUpCP0->tlb_refill_stackPtr = (memaddr) KERNELSTACK;
    passUpCP0->exception_handler = (memaddr)  exceptionHandler;
    passUpCP0->exception_stackPtr = (memaddr) KERNELSTACK;


    /* data structures initialization */
    initPcbs();
    initASH();
    initNamespaces();

    /* nucleus variables initialization */
    readyPCB=0;
    processCount=0;
    SBcount=0;
    mkEmptyProcQ(&readyQueue); 
    currentProcess=NULL;
    for (int i=0;i<8;i++){
        //deviceSem[i]=0;
        flashSem[i] = 0;
        netSem[i] = 0;
        printerSem[i] = 0;
        termSem[i] = 0;
        termSem[i+8] = 0;
    }
    pseudoClockSem=0;

    /* set Interval Timer to 100 ms */
    LDIT(PSECOND);

    /* First Process initialization */
    pcb_t* init = allocPcb();
    insertProcQ(&readyQueue,init);
    readyPCB++;
    processCount++;
    init->p_time=0;
    init->p_supportStruct=NULL;
    init->p_pid = 1;
    init->valueAddr = NULL;
    /* semADD and Process Tree fields initializated in allocPcb() */
    
    /* set the init' status: 
    IEp - Interrupt Enabled, 
    KUp - Kernel Mode on -> KUp = 0, 
    IM - Interrupt Mask all set to 1 , 
    TE - processor Local Timer enabled */
    init-> p_s.status =  (ALLOFF | (IEPON | IMON | TEBITON)) ;

    /* set PC to the address of test (assign in t9 reg for tecnical reason) */
    init->p_s.pc_epc = init->p_s.reg_t9 = (memaddr) test;
    
    /* set SP to RAMTOP */
    RAMTOP(init->p_s.reg_sp);

    /* call the scheduler */
    schedule();

    return 0;
}