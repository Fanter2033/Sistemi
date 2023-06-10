#include "initial.h"

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

    /* first Process initialization */
    pcb_t* init = allocPcb();
    insertProcQ(&readyQueue,init);
    processCount++;
    init->p_time=0;
    init->p_supportStruct=NULL;
    init->p_pid = PID;
    init->valueAddr = NULL;
    /* semADD and Process Tree fields initializated in allocPcb() */
    
    /* set the firstProcess status: 
        IEp - Interrupt Enabled, 
        KUp - Kernel Mode on (0), 
        IM - Interrupt Mask all set to 1 , 
        TE - processor Local Timer enabled */
    init-> p_s.status =  (ALLOFF | (IEPON | IMON | TEBITON)) ;

    /* set PC to the address of test (assign in t9 reg for tecnical reason) */
    init->p_s.pc_epc = (memaddr) test;
    init->p_s.reg_t9 = (memaddr) test;
    
    /* set SP to RAMTOP */
    RAMTOP(init->p_s.reg_sp);

    /* call the scheduler */
    scheduler();

    return 0;
}