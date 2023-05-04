#ifndef ASH_H
#define ASH_H


#include "pandos_types.h"
#include "pandos_const.h"
#include "hashtable.h"
#include "pcb.h"



/* Additional function : if the semaphore's list of proc. is empty remove it from the ash*/
void removeEmptySemd(semd_t* s);

/* add PCB pointed by p in the SEMD blocked process with semAdd as key */
int insertBlocked(int *semAdd, pcb_t *p);

/* return the first pcb from the queue of proc. of SEMD with semAdd as key, 
   remove the relative descriptor from the ASH (and relink it in the semdFree_h) */
pcb_t* removeBlocked(int *semAdd);

/* remove the PCB pointed by p (and eventually the semaphore descriptor when the queue is empty) 
   from the queue which is blocked (p->semAdd)
   if (there is no pcb ) return NULL else return p */
pcb_t* outBlocked(pcb_t* p);

/* return (without remove) the PCB pointer located in the head of the SEMD proc. queue 
   if (there isn't the SEMD in ASH or its proc queue is empty) */
pcb_t* headBlocked(int *semAdd);

/* initialize semdFree_h using an array of semd_t */
void initASH();


#endif