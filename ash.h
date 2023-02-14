#ifndef ASH_H
#define ASH_H

#include "pandos_types.h"
#include "pandos_const.h"
#include "hashtable.h"
#include "pcb.h"

HIDDEN DECLARE_HASHTABLE(semd_h,5);
HIDDEN struct list_head semdFree_h = LIST_HEAD_INIT(semdFree_h);

static void removeEmptySemd(semd_t* s);

/* add PCB pointed by p in the SEMD blocked process with semAdd as key */
int insertBlocked(int *semAdd, pcb_t *p);

/* return the first pcb from the queue of proc. of SEMD with semAdd as key, 
   remove the relative descriptor from the ASH (and relink it in the semdFree_h) */
pcb_t* removeBlocked(int *semAdd);

/* remove the PCB pointed by p (and eventually the semaphore descriptor when the queue is empty) from the queue which is blocked (p->semAdd), 
if (there is no pcb ) return NULL else return p*/
pcb_t* outBlocked(pcb_t* p);

/* return (without remove) the PCB pointer located in the head of the SEMD proc. queue 
 if (there isn't the SEMD in ASH || its proc queue is empty) */
pcb_t* headBlocked(int *semAdd);

/* initialize semdFree_h using an array of semd_t 
 * semd_t linked using member: s_freelink
 */
void initASH();


#endif