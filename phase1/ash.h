#ifndef ASH_H
#define ASH_H

#include "pcb.h"
#include "../h/hashtable.h"

/* additional function: removes (if empty) the semaphore's list of proc. from the ash*/
void removeEmptySemd(semd_t* s);

/* add p to the SEMD's blocked proc. queue with semAdd as key 
   returns 1 if the new SEMD can't be allocated, 0 otherwise */
int insertBlocked(int *semAdd, pcb_t *p);

/* return (and remove) the first pcb in SEMD's blocked proc. queue with semAdd as key, 
   NULL if the descriptor isn't found. If the queue is empty removes the
   relative descriptor from the ASH (and relink it in the semdFree_h) */
pcb_t* removeBlocked(int *semAdd);

/* remove p (and eventually the semaphore descriptor when the queue is empty) 
   from the queue on which is blocked (p->semAdd) 
   return NULL if the pcb isn't found otherwise p itself */
pcb_t* outBlocked(pcb_t* p);

/* return (without removing it) the first pcb in SEMD proc. queue  with semAdd as key,
   NULL if there isn't the SEMD in ASH or its proc queue is empty */
pcb_t* headBlocked(int *semAdd);

/* initialize semdFree_h using an array of semd_t */
void initASH();

/* additional function for phase2: search a pcb in ready queue and in semaphores via its pid 
   and returns it (NULL if it isn't found) */
pcb_t* findPCB_pid(int pid, struct list_head *queue);

#endif