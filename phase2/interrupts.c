#include "pcb.h"
#include "ns.h"
#include "ash.h"
#include <pandos_const.h>
#include <pandos_types.h>
#include <umps3/umps/cp0.h>
#include <umps3/umps/libumps.h>

/* take T and N and return the n-th bit of T */
#define IPLINE(T,N) ((T & (1U << N)) >> N) //non so se si può fare
#define PLT 1
#define INT 2

void interruptHandler(){
    /* take causeIP part as integer */
    unsigned int causeIP = ((getCAUSE() & CAUSE_IP_MASK) >> CAUSE_IP_BIT(0));
    int line = 1; /* line 0 not considered (NO multiprocessor) */

    /* check goes from most significant bit to lower, so the priority is considered*/
    while (causeIP != 0){   /* loop to resolve all interrupts */

        if (IPLINE(causeIP,line) == ON){     //INTERRUPT LINE ON
            switch (line){
            case PLT:
                //3.6.2
                break;
            
            case INT:
                //3.6.3
                break;

            default:
                nonTimerInterruptHandler(line);
                break;
            }
        }
        line++;
    }
}

void nonTimerInterruptHandler(int interruptLine){
    
    switch (interruptLine){
        case DISKINT:
            break;

        case FLASHINT:
            break;
        
        case NETWINT:
            break;
        
        case PRINTINTERRUPT:
            break;
        
        case TERMINT:
            break;
        
        default:
            break;
    }
}