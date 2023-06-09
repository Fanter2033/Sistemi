#ifndef PCB_H
#define PCB_H

#include "../h/pandos_types.h"
#include "../h/pandos_const.h"
#include "../h/list.h"

/* initialize the pcbFree list */
void initPcbs();

/* adds the PCB pointed by p to pcbFree_h */
void freePcb(pcb_t* p);

/* removes an element from pcbFree_h, returns NULL if pcbFree_h is empty*/
pcb_t* allocPcb();


/* -- Queue list management -- */

/* creates an empty PCB list */
void mkEmptyProcQ(struct list_head *head);

/* tests if PCB list is empty (true if empty)*/
int emptyProcQ(struct list_head *head);

/* adds p to the PCB list head */
void insertProcQ(struct list_head* head, pcb_t* p);

/* returns the head element from PCB queue (without removing it), NULL otherwise */
pcb_t* headProcQ(struct list_head* head);

/* removes and return the first element from PCB queue, (NULL if empty) */
pcb_t* removeProcQ(struct list_head* head);

/* removes and return the PCB pointed by p from the PCB queue, return NULL if P isn't in the queue */
pcb_t* outProcQ(struct list_head* head, pcb_t* p);



/* -- Tree management -- */

/* tests whether p has any children (true if empty)*/
int emptyChild(pcb_t *p);

/* inserts p as children of prnt */
void insertChild(pcb_t* prnt, pcb_t* p);

/* removes and return the first child of p (NULL if it doesn't exist) */
pcb_t* removeChild(pcb_t* p);

/* removes p from the children list of its parent (NULL if it hasn't one) and returns it*/
pcb_t* outChild(pcb_t* p);

/*additional function for phase2: search a PCB in a queue via its pid 
  and returns it (NULL if it isn't in the queue) */
pcb_t* findPCBfromQUEUE(int pid, struct list_head* head );


#endif //PCB_H