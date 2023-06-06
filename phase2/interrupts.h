#ifndef INTERRUPTS_H
#define INTERRUPTS_H

#include <utils.h>

/* Takes T and N and return the n-th bit of T */
#define NBIT(T,N) ((T & (1 << N)) >> N) 

/* Returns the "interrupt pending" bits of the cause register */
#define CAUSEIP ((getCAUSE() & CAUSE_IP_MASK) >> CAUSE_IP_BIT(0)) 

/* Hardware constant */
#define TERMSTATMASK 0xFF

/* ---- Functions Declaration ---- */

void interruptHandler();

/* Performs a V for every process blocked on the pseudoClock semaphore and 
    puts the unblocked processes on the readyQueue*/
void ITInterrupt();

/* Handles the processor local timer interrupt and so it moves the 
    currentProcess to the readyQueue */
void PLTinterrupt();

/* Calculates which device has caused the interrupt and calls a function to resolve it*/
void nonTimerInterruptHandler(int interruptLine);

/* Acknowledges the interrupt and unlocks (if there is) the process 
    from the correct terminal semaphore (and puts it in the readyQueue)*/
void resolveTerm(int line, int device);

/* Same as resolveTerm except that this is for every other device */
void resolveNonTerm(int line, int device);

/* -- Additional Functions -- */

/* Performs a P on device semaphore at sem */
void P(int* sem);

/* Performs a V on device semaphore at sem */
pcb_t* V(int* sem);

#endif