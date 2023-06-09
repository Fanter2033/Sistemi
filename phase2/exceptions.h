#ifndef EXCEPTIONS_H
#define EXCEPTIONS_H

#include <utils.h>

#define GENERALERROR   -1 

/* Checks if an address is a devices' one */
#define NOTDEV(cmdAddr) (int)cmdAddr < (memaddr)DEV_REG_START || (int)cmdAddr >= (memaddr) DEV_REG_END

/* Calculate the correct interrupt line from an address */
#define LINEDEV(cmdAddr) (((int)cmdAddr - (int)DEV_REG_START) / (DEV_REG_SIZE*DEVPERINT)) + DEV_IL_START

/* ---- Global Variables ---- */

state_t* BIOSDPState; /* BIOS Data Page exception State */

/* ---- Functions Declaration ---- */

void exceptionHandler();
void syscallExcHandler();

/*-- Syscalls --*/

/* Creates a new process and sets its state, support structure and
 namespace (if ns is NULL its set to the parent one) */
int createProcess(state_t *statep, support_t *supportp, nsd_t *ns);

/* Terminates either the current process or the process with pid "pid" 
and (recursively) their progeny */
void terminateProcess(int pid);

/* Performs a P operation on the semaphore pointed by sem */
bool Passeren(int *sem);

/* Performs a V operation on the semaphore pointed by sem */
bool Verhogen(int *sem);

/* Performs an Input/Output operation on device at cmdAddress with the values in cmdValues. 
    Returns 0 if the operation is successful*/
int DO_IO(int *cmdAddr, int *cmdValues);

/* Returns the processor time used by the requesting process */
cpu_t getTime();

/* Performs a P on the pseudoClock semaphore*/
void waitForClock();

/* Returns a pointer to the support structure of the requesting process */
support_t* getSupportData();

/* Returns the PID either of the requesting process or its parent one 
    (it returns 0 if parent and child are not in the same namespace)*/
int getProcessID(int parent);

/* Returns the number of children in the same PID ns as the requesting process, furthermore
 it loads the "children" array with the process' children until size "size" */
int getChildren(int *children, int size);

/*-- Additional Functions --*/

/* Returns the correct interrupt line from the device address */
int findLine(int* cmdAddr);

/* Returns the (sub)device number (0-15 for terminal, 0-7 for every other device) */
int findDevice(int line, int* cmdAddr); 

/* Handles Syscall over 11, program trap and TLB exceptions either terminating the process 
(support structure is NULL) or passing up the control to the Support Level */
void passUporDie(int indexValue);

/* Sets the currentProcess' time */
void updateCPUtime();

/* Additional function that copies n characters from memory area src to memory area dest */
void memcpy(void *dest, const void *src, unsigned long n);

#endif