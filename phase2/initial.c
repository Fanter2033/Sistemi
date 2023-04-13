#include "pcb.h"
#include "ns.h"
#include "ash.h"
#include <pandos_const.h>
#include <pandos_types.h>
#include <umps3/umps/libumps.h>

#define ALDEV 64        
/*
    All Lines Devices:

    0...7 -> PLT
    8...15 -> Intterval Timer
    16...23 -> Disk Devices
    24...31 -> Flash Devices
    32...39 -> Network Devices
    40...47 -> Printer Devices
    48...55 -> Terminal Devices (W)
    56...63 -> Terminal Devices (R)
    
*/

int processCount;   /*process started but not yet terminated */
int SBcount;    /* soft-blocked count */
list_head* readyQueue;  /* queue of ready pcb */
pcb_t* currentProcess;  /* pcb that is in running state */

int deviceSem[ALDEV];      

int pseudoClockSem;

passupvector_t *passUpCP0;

extern void test();

void uTLB_RefillHandler(){
    setENTRYHI(0x80000000);
    setENTRYLO(0x00000000);
    TLBWR();
    LDST((STATE_PTR) 0x0FFFF000);
}

int main(){
    /* passup vector initialization */
    passUpCP0->tlb_refill_handler = (memaddr) uTLB_RefillHandler;
    passUpCP0->tlb_refill_stackPtr = (memaddr) 0x20001000;
    passUpCP0->exception_handler = (memaddr) FUNZIONEDAIMPLEMENTARE;
    passUpCP0->exception_stackPtr = (memaddr) 0x20001000;

    /* data structures initialization */
    initPcbs();
    initASH();
    initNamespaces();

    /* nucleus variables initialization */
    processCount=0;
    SBcount=0;
    mkEmptyProcQ(readyQueue);
    currentProcess=NULL;
    for (int i=0;i<ALDEV;i++){
        deviceSem[i]=0;
    }
    pseudoClockSem=0;

    /* First Process initialization */
    pcb_t* init = allocPcb();
    insertProcQ(readyQueue,init);
    processCount++;
    init->p_time=0;
    init->p_supportStruct=NULL;
    /* semADD and Process Tree fields initializated in allocPcb() */


    /*
        per controllare se un bit è a 1 o 0:

        se io voglio il k-esimo bit di n bit totali, devo fare:
        (n & (1 << k)) >> k

        esempio: 7 -> 0111
        voglio il bit 2 di 4 totali
        0100 & (1 << 2) >> 2 --> 0100 & 0100 >> 2 --> 0100 >> 2 --> 1
        voglio il bit 1 di 4 totali
        0100 & 0010 >> 2 --> 0000 >> 2 --> 0


        per settarlo: 
        uso il numero di base (in binario) e faccio l'or bit per bit di quello che voglio mettere a 1
        1000|1<<2 --> 1000|0100--> 1100

        in init io voglio: 

        - kernel-mode ON (IEp e KUp)
            sono i bit 2 e 3 
            | 1100 = 12

        - interrupt abilitati
            IM -> bit 8 - 15
            quindi:
            1111 1111 0000 1100 -> 65292

        - processor Local Timer abilitato
            è il bit 27
            1111 1111 0000 1100
            

        - SP settato a RAMTOP
        - PC settato a TEST
    */

    /* set IEp & KUp and all interrupts enabled */

    init->p_s.status | 65292;

    /* set processor Local Timer */
    init->p_s.status | 1 << 27;

    //init->p_s.status.TE = 1; ?? vedi riferimento sul libro, pagina 9

    /* set PC */
    init->p_s.pc_epc = (memaddr) test;
    init->p_s.reg_t9 = (memaddr) test;

    /* set SP */
    
    init->p_s.reg_sp = (memaddr) RAMTOP;

    //call the scheduler
}