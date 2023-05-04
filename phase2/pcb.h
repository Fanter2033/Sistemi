#ifndef PCB_H
#define PCB_H

#include "pandos_types.h"
#include "pandos_const.h"
#include "list.h"



/* init the pcbFree list */
void initPcbs();

/* add the element pointed by p in pcbFree_h */
void freePcb(pcb_t* p);

/* if pcbFree is empty then NULL else remove an element from pcbFree_h */
pcb_t* allocPcb();


/* -- Queue list management -- */

/* create a empty pcb list */
void mkEmptyProcQ(struct list_head *head);

/* if PCB list is empty then TRUE, FALSE otherwise  */
int emptyProcQ(struct list_head *head);

/* insert p's pointer into PCB queue */
void insertProcQ(struct list_head* head, pcb_t* p);

/* return the head element from PCB queue, NULL otherwise */
pcb_t* headProcQ(struct list_head* head);

/* removes and return the first element from PCB queue, return NULL if the queue is empty*/
pcb_t* removeProcQ(struct list_head* head);

/* removes the PCB pointed by p from the PCB queue, if P isn't in the queue then return NULL*/
pcb_t* outProcQ(struct list_head* head, pcb_t* p);



/* -- Tree management -- */

/* return (PCB pointed by p has children) */
int emptyChild(pcb_t *p);

/* insert the PCB pointed by p as child of PCB pointed by prnt */
void insertChild(pcb_t* prnt, pcb_t* p);

/* if (p has children) then return and remove its first child else return NULL */
pcb_t* removeChild(pcb_t* p);

/* if (PCB pointed by p has a father) remove it else return NULL (this PCB could be in any position)*/
pcb_t* outChild(pcb_t* p);

#endif //PCB_H