#include "pcb.h"
#include "ns.h"
#include "ash.h"
#include "list.h"
#include <pandos_const.h>
#include <pandos_types.h>
#include <umps3/umps/libumps.h>
#include <umps3/umps/cp0.h>

#define NBIT(T,N) ((T & (1 << N)) >> N) 
#define ALDEV 50  

/*
    All Lines Devices:

    0 -> PLT
    1 -> Interval Timer
    2...9 -> Disk Devices
    10...17 -> Flash Devices
    18...25 -> Network Devices
    26...33 -> Printer Devices
    //in 3.1, ultimo punto dice di avere un semaforo per ogni sub-device
    //8 terminal -> 16 sub-device
    34...49 -> Terminal Devices:
        34 .. 41 -> Terminal - (W)
        42 .. 49 -> Terminal - (R)

*/

int processCount;   /*process started but not yet terminated */
int SBcount;    /* soft-blocked count */
pcb_t* currentProcess;  /* pcb that is in running state */
struct list_head readyQueue;  /* queue of ready pcb */

int deviceSem[ALDEV];      

int pseudoClockSem;

int processStartTime;

passupvector_t* passUpCP0;

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
    processCount=0;
    SBcount=0;
    mkEmptyProcQ(&readyQueue); 
    currentProcess=NULL;
    for (int i=0;i<ALDEV;i++){
        deviceSem[i]=0;
    }
    pseudoClockSem=0;

    /* set Interval Timer to 100 ms */
    LDIT(PSECOND);

    /* First Process initialization */
    pcb_t* init = allocPcb();
    insertProcQ(&readyQueue,init);
    processCount++;
    init->p_time=0;
    init->p_supportStruct=NULL;
    init->p_pid = 1;
    /* semADD and Process Tree fields initializated in allocPcb() */
    
    /* set the init' status: 
    IEp - Interrupt Enabled, 
    KUp - Kernel Mode on -> KUp = 0, 
    IM - Interrupt Mask all set to 1 , 
    TE - processor Local Timer enabled */
    init-> p_s.status =  (ALLOFF | (IEPON | IMON | TEBITON)) ;

    /* set PC to the address of test (assign in t9 reg for tecnical reason) */
    init->p_s.pc_epc = (memaddr) test;
    init->p_s.reg_t9 = (memaddr) test;
    
    /* set SP to RAMTOP */
    RAMTOP(init->p_s.reg_sp);

    //addokbuf("kernel started\n");

    /* call the scheduler */
    schedule();

    return 0;
}