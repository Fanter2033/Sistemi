#include "pcb.h"
#include "ns.h"
#include "ash.h"
#include <pandos_const.h>
#include <pandos_types.h>
#include <umps3/umps/cp0.h>
#include <umps3/umps/libumps.h>


extern int processCount;
extern int SBcount;
extern struct list_head* readyQueue;
extern pcb_t* currentProcess;

/*
    per tenere traccia dei PID, io userei un semplice intero progressivo
*/


void exceptionHandler(){

   /* save processor state from BIOS Data Page */
   currentProcess->p_s = (*((state_t *) BIOSDATAPAGE));

    unsigned int excCode = CAUSE_GET_EXCCODE(getCAUSE()); // from CPU or PCB ?? 
    if (excCode == 0){
        //controllo passa all'Interrupt Handler
    }
    else if (excCode == 8){
        syscallExcHandler();
    }
    else if (excCode < 4){  //1-2-3
        //controllo passa al TLB Exception Handler (pass up or die)
    }
    else{   /* 4-7 o 9-12 */
        //controllo passa al Program Trap Exception Handling (pass up or die)
    }
}

void syscallExcHandler(){
    unsigned int a0 = currentProcess ->p_s.reg_a0;

    switch (a0) {
    case CREATEPROCESS:
        //createProcess(a1,a2,a3);
        break;
    case TERMPROCESS:
        //terminateProcess(a1);
        break;
    case PASSEREN:
        //Passeren(a1);
        break;
    case VERHOGEN:
        //Verhogen(a1);
        break;
    case IOWAIT:
        //DO_IO(a1,a2);
        break;
    case GETTIME:
        //getCpuTime();
        break;
    case CLOCKWAIT:
        //waitForClock():
        break;
    case GETSUPPORTPTR:
        //getSupportData();
        break;
    case TERMINATE:
        //getProcessID(a1);
        break;
    case GET_TOD:
        //getChildren(a1,a2);
        break;
    
    default:        // > 11

        break;
    }

    
    //incrementare il PC prima di ritornare, altrimenti LOOP sulla syscall
}