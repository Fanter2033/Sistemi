#include "pcb.h"
#include "ns.h"
#include "ash.h"
#include <pandos_const.h>
#include <pandos_types.h>

#include <umps3/umps/libumps.h>


extern int processCount;
extern int SBcount;
extern struct list_head* readyQueue;
extern pcb_t* currentProcess;

/*
    per tenere traccia dei PID, io userei un semplice intero progressivo
*/


void exceptionHandler(){
    /* 
        credo vada salvato lo stato del processo in quel momento attivo, altrimenti andrebbe perso tutto? 
        nella guida (punto 3.4) c'è scritto che al momento dell'eccezione, lo stato del processo viene salvato in 
        0x0FFFF000
        Quindi credo vada preso in qualche modo da lì
    */
    unsigned int excCode = GETEXECCODE(getCAUSE());
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
    /*
        NON SO DOVE MINCHIA PRENDERE il contenuto di a0

    */

    unsigned int a0 = ... ;
    switch (a0)
    {
    case 1:
        //createProcess(a1,a2,a3);
        break;
    case 2:
        //terminateProcess(a1);
        break;
    case 3:
        //Passeren(a1);
        break;
    case 4:
        //Verhogen(a1);
        break;
    case 5:
        //DO_IO(a1,a2);
        break;
    case 6:
        //getCpuTime();
        break;
    case 7:
        //waitForClock():
        break;
    case 8:
        //getSupportData();
        break;
    case 9:
        //getProcessID(a1);
        break;
    case 10:
        //getChildren(a1,a2);
        break;
    
    default:        // > 11

        break;
    }

    
    //incrementare il PC prima di ritornare, altrimenti LOOP sulla syscall
}